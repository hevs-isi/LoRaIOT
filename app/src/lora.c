#include "lora.h"
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lora, LOG_LEVEL_DBG);

#ifdef CONFIG_LORA_IM881A
#include <wimod_lorawan_api.h>
#endif

#ifdef CONFIG_BOARD_NRF52840_LORAIOT
#include <board_utils.h>
#endif

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
	LOG_INF("LoRaWAN Network joined.\n");
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

#ifdef CONFIG_BOARD_NRF52840_LORAIOT
#define LORA_UART_DEV CONFIG_LORA_IM881A_UART_DRV_NAME
#elif defined(CONFIG_BOARD_NUCLEO_L432KC)
#define LORA_UART_DEV "UART_1"
#endif

void lora_init()
{
	uart = device_get_binding(LORA_UART_DEV);
	uart_cfg = (struct uart_device_config *)(uart->config->config_info);

	/* BUG: couldn't launch the debug when the function 'uart_irq_rx_enable' is called too quickly */
	k_busy_wait(1000000);

	#ifdef CONFIG_BOARD_NRF52840_LORAIOT
	VDDH_ACTIVATE();
	#endif

	wimod_lorawan_init();

	wimod_lorawan_join_network_request(join_callback);

	k_work_q_start(&lora_msg_work_q, lora_msg_stack,
		   K_THREAD_STACK_SIZEOF(lora_msg_stack), K_PRIO_PREEMPT(0));

	k_work_init(&lmsg.work, lora_msg_send);

	disable_uart();
}
