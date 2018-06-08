
#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <console/uart_pipe.h>

#include "im881a.h"

static struct device *lora_im881a_dev;

void im881a_init()
{
	lora_im881a_dev = device_get_binding(CONFIG_LORA_IM881A_UART_DRV_NAME);
	//lora_im881a_dev = device_get_binding(CONFIG_SLIP_DRV_NAME);
}


void im881_send()
{
	//uart_poll_out(lora_im881a_dev, 'a');

	//lora_im881a_dev->driver_api->
	u8_t buf[1] = { 'a' };

	uart_pipe_send(&buf[0], 1);

}
