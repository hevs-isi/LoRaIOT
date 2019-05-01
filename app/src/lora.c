#include "lora.h"
#include <zephyr.h>

#include <logging/log.h>
#include <device.h>
#include <counter.h>
#include "global.h"
#include <pinmux/stm32/pinmux_stm32.h>

LOG_MODULE_REGISTER(lora, LOG_LEVEL_DBG);

#ifdef CONFIG_LORA_IM881A
#include <wimod_lorawan_api.h>
#endif

#ifdef CONFIG_BOARD_NRF52840_LORAIOT
#include <board_utils.h>
#endif

#include <gpio.h>

static struct device *uart;
static struct uart_device_config *uart_cfg;

K_THREAD_STACK_DEFINE(lora_msg_stack, 1024);
static struct k_work_q lora_msg_work_q;

struct lora_msg {
	u8_t port;
	u8_t data[20];
	u8_t size;
	struct k_work work;
};

static struct lora_msg lmsg;

#include <uart.h>

void join_callback()
{
	LOG_INF("LoRaWAN Network started.\n");
	//blink_led(LED_GREEN, MSEC_PER_SEC/4, K_SECONDS(3));
	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
		VDDH_DEACTIVATE();
	#endif
}

void disable_uart()
{
	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
	if( NOT_IN_DEBUG() ){
		*(uart_cfg->base + 0x500) = 0;
	}
	#endif
}

void enable_uart()
{
	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
	if( NOT_IN_DEBUG() ){
		*(uart_cfg->base + 0x500) = 4UL;
	}
	#endif
}

static void lora_msg_send(struct k_work *item)
{
	struct lora_msg *msg = CONTAINER_OF(item, struct lora_msg, work);

	enable_uart();
	wimod_lorawan_send_c_radio_data(msg->port, msg->data, msg->size);
	disable_uart();
	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
	VDDH_DEACTIVATE();
	#endif
}

void lora_init()
{
	uart = device_get_binding(CONFIG_LORA_IM881A_UART_DRV_NAME);
	uart_cfg = (struct uart_device_config *)(uart->config->config_info);

	/* BUG: couldn't launch the debug when the function 'uart_irq_rx_enable' is called too quickly */
	k_busy_wait(1000000);

	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
	VDDH_ACTIVATE();
	#endif

	wimod_lorawan_init();

	k_work_q_start(&lora_msg_work_q, lora_msg_stack,
		   K_THREAD_STACK_SIZEOF(lora_msg_stack), K_PRIO_PREEMPT(0));

	k_work_init(&lmsg.work, lora_msg_send);

	wimod_lorawan_join_network_request(join_callback);

	disable_uart();
}

void lora_off()
{
	int ret;
	struct device *dev;

	/* Power off radio module */
	dev = device_get_binding(DT_GPIO_STM32_GPIOD_LABEL);
	ret = gpio_pin_configure(dev, 2, (GPIO_DIR_OUT));
	ret = gpio_pin_write(dev, 2, 0);

	/* configure uart pins as inputs */
	dev = device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
	ret = gpio_pin_configure(dev, 9, (GPIO_DIR_IN));
	ret = gpio_pin_configure(dev, 10, (GPIO_DIR_IN));
}

static const struct pin_config pinconf_lora_uart[] = {
	{STM32_PIN_PA9, STM32L4X_PINMUX_FUNC_PA9_USART1_TX},
	{STM32_PIN_PA10, STM32L4X_PINMUX_FUNC_PA10_USART1_RX},
};

void lora_on()
{
	int ret;
	struct device *dev;

	/* Power on radio module */
	dev = device_get_binding(DT_GPIO_STM32_GPIOD_LABEL);
	ret = gpio_pin_configure(dev, 2, (GPIO_DIR_OUT));
	ret = gpio_pin_write(dev, 2, 1);

	stm32_setup_pins(pinconf_lora_uart, ARRAY_SIZE(pinconf_lora_uart));
}

void lora_time_AppTimeReq(u8_t AnsRequired)
{
	struct device *counter_dev;
	counter_dev = device_get_binding(DT_RTC_0_NAME);
	u32_t time = counter_read(counter_dev);

	u8_t TokenReq = global.lora_TokenReq;
	global.lora_TokenReq++;
	global.lora_TokenReq&=0x1f;

	const u8_t data[] =
	{
		0x01, // cid
		time >> 24,
		time >> 16,
		time >> 8,
		time >> 0,
		(AnsRequired ? (1 << 5) : 0) | (TokenReq & 0x1f)
	};

	wimod_lorawan_send_u_radio_data(202, data, sizeof(data));
}
