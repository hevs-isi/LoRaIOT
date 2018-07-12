
#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <console/uart_pipe.h>

#include "im881a.h"
#include <wimod_hci_driver.h>

static struct device *lora_im881a_dev;

u8_t pipe_buf[1];
wimod_hci_message_t rx_msg;


static u8_t *upipe_rx(u8_t *buf, size_t *off)
{
	return 0;
}

static wimod_hci_message_t* recv_cb(wimod_hci_message_t* rx_msg)
{
	return rx_msg;
}


void im881a_init()
{
	//lora_im881a_dev = device_get_binding(CONFIG_LORA_IM881A_UART_DRV_NAME);
	//lora_im881a_dev = device_get_binding(CONFIG_SLIP_DRV_NAME);

	//uart_pipe_register(pipe_buf, 1, upipe_rx);

	wimod_hci_init(recv_cb, &rx_msg);

}


void im881_send()
{

	wimod_hci_message_t tx_msg;
	//uart_poll_out(lora_im881a_dev, 'a');

	//lora_im881a_dev->driver_api->
	//u8_t buf[1] = { 'a' };

	tx_msg.sap_id     = 0x01;
	tx_msg.msg_id     = 0x01;
	tx_msg.length     = 0;


	//uart_pipe_send(&buf[0], 1);
	wimod_hci_send_message(&tx_msg);

}
