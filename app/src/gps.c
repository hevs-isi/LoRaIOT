#include "gps.h"
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(gps, LOG_LEVEL_DBG);

static struct device *uart;
static struct uart_device_config *uart_cfg;

#include <uart.h>
#include <gpio.h>

#include <pinmux.h>
#include <sys_io.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* manual pin assignments*/
static const struct pin_config pinconf[] = {
	{STM32_PIN_PC4, STM32L4X_PINMUX_FUNC_PC4_USART3_TX},
	{STM32_PIN_PC5, STM32L4X_PINMUX_FUNC_PC5_USART3_RX},
};

static K_FIFO_DEFINE(gps_fifo);
struct data_item_t {
	void *fifo_reserved;   /* 1st word reserved for use by fifo */
	char data[100];
};

static struct data_item_t isr_buffer;
static u32_t isr_index = 0;

static void gps_uart_isr(struct device *unused)
{
	ARG_UNUSED(unused);

	while (uart_irq_update(uart) && uart_irq_is_pending(uart))
	{

		if (!uart_irq_rx_ready(uart)) {
			if (uart_irq_tx_ready(uart)) {
				LOG_DBG("transmit ready");
			} else {
				LOG_DBG("spurious interrupt");
			}
			/* Only the UART RX path is interrupt-enabled */
			break;
		}

		int n;

		for (;;)
		{
			uint8_t buf;
			n = uart_fifo_read(uart, &buf, sizeof(buf));
			if (n <= 0) break;

			if (isr_index > ARRAY_SIZE(isr_buffer.data) - 1)
			{
				isr_index = 0;
			}

			if (buf == '\r')
			{
				continue;
			}

			if (buf == '\n')
			{
				isr_buffer.data[isr_index++] = 0;
				isr_index = 0;
				k_fifo_put(&gps_fifo, &isr_buffer);
				continue;
			}

			isr_buffer.data[isr_index++] = buf;
		}
	}
}

void gps_thread(void *u1, void *u2, void *u3)
{
	ARG_UNUSED(u1);
	ARG_UNUSED(u2);
	ARG_UNUSED(u3);

	int ret;
	struct device *dev;

	struct uart_config c = {
		.baudrate	= 9600,
		.parity		= UART_CFG_PARITY_NONE,
		.stop_bits	= UART_CFG_STOP_BITS_1,
		.data_bits	= UART_CFG_DATA_BITS_8,
		.flow_ctrl	= UART_CFG_FLOW_CTRL_NONE,
	};

	/*usart pins*/
	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	uart = device_get_binding("UART_3");
	LOG_DBG("uart = %p", uart);
	uart_cfg = (struct uart_device_config *)(uart->config->config_info);
	if (!uart) {
		printk("Error uart not found");
	}

	ret = uart_configure(uart, &c);
	if (ret) {
		printk("Error uart config failed");
	}

	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);

	uart_irq_callback_set(uart, gps_uart_isr);

	uart_irq_rx_enable(uart);

	/* Power on gps module */
	dev = device_get_binding(DT_GPIO_STM32_GPIOC_LABEL);
	ret = gpio_pin_configure(dev, 13, (GPIO_DIR_OUT));
	if (ret) {
		printk("Error configuring pin PC%d!\n", 13);
	}

	ret = gpio_pin_write(dev, 13, 1);
	if (ret) {
		printk("Error set pin PC%d!\n", 13);
	}

	/* backup on gps module */
	if (1)
	{
		dev = device_get_binding(DT_GPIO_STM32_GPIOA_LABEL);
		ret = gpio_pin_configure(dev, 12, (GPIO_DIR_OUT));
		if (ret) {
			printk("Error configuring pin PA%d!\n", 12);
		}

		ret = gpio_pin_write(dev, 12, 1);
		if (ret) {
			printk("Error set pin PA%d!\n", 12);
		}
	}

	for (;;)
	{
		struct data_item_t *buffer = k_fifo_get(&gps_fifo, K_FOREVER);

		LOG_DBG("gps rx:'%s'", buffer->data);
	}
}

#if 0
K_THREAD_DEFINE(gps_tid, 1024,
                gps_thread, NULL, NULL, NULL,
                5, 0, K_NO_WAIT);
#endif

void gps_init(void)
{

}
