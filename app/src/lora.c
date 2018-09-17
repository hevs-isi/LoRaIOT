
#include <zephyr.h>
#include <shell/shell.h>

#include <wimod_lorawan_api.h>

#define SYS_LOG_DOMAIN			"lora"
#include <logging/sys_log.h>

#include <board_utils.h>
#include <uart.h>

#include "lora.h"

static struct device *uart;
static struct uart_device_config *uart_cfg;

static int shell_cmd_info(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_firmware_version();

	return 0;
}

static int shell_cmd_get_rtc(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rtc();

	return 0;
}

static int shell_cmd_set_rtc(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_rtc();

	return 0;
}

static int shell_cmd_get_rtc_alarm(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rtc_alarm();

	return 0;
}

static int shell_factory_reset(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_factory_reset();

	return 0;
}

static int shell_cmd_deveui(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_device_eui();

	return 0;
}

static int shell_cmd_setparam(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_join_param_request();

	return 0;
}

static int shell_cmd_join(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_join_network_request(NULL);

	return 0;
}

static int shell_nwk_status(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_nwk_status();

	return 0;
}

static int shell_set_rstack_cfg(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_rstack_config();

	return 0;
}

static int shell_rstack_cfg(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rstack_config();

	return 0;
}

static int shell_send_udata(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	u8_t data[3];

	data[0] = 0x11;
	data[1] = 0x22;
	data[2] = 0x33;

	wimod_lorawan_send_u_radio_data(1, data, 3);

	return 0;
}

static int shell_send_cdata(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	u8_t data[3];

	data[0] = 0x11;
	data[1] = 0x22;
	data[2] = 0x33;

	wimod_lorawan_send_c_radio_data(1, data, 3);

	return 0;
}


static struct shell_cmd commands[] = {
	{ "firmware", shell_cmd_info, NULL },
	{ "get_rtc", shell_cmd_get_rtc, NULL },
	{ "set_rtc", shell_cmd_set_rtc, NULL },
	{ "get_rtc_alarm", shell_cmd_get_rtc_alarm, NULL },
	{ "factory", shell_factory_reset, NULL },
	{ "deveui", shell_cmd_deveui, NULL },
	{ "set", shell_cmd_setparam, NULL },
	{ "join", shell_cmd_join, NULL },
	{ "nwk", shell_nwk_status, NULL },
	{ "setrstack", shell_set_rstack_cfg, NULL },
	{ "rstack", shell_rstack_cfg, NULL },
	{ "set_rstack", shell_set_rstack_cfg, NULL },
	{ "udata", shell_send_udata, NULL },
	{ "cdata", shell_send_cdata, NULL },
	{ NULL, NULL, NULL }
};

void disable_uart()
{
	if( NOT_IN_DEBUG() ){
		*(uart_cfg->base + 0x500) = 0;
	}
}

void enable_uart()
{
	if( NOT_IN_DEBUG() ){
		*(uart_cfg->base + 0x500) = 4UL;
	}
}

void join_callback()
{
	SYS_LOG_INF("LoRaWAN Network joined.\n");
	//blink_led(LED_GREEN, MSEC_PER_SEC/4, K_SECONDS(3));
}



void lora_init()
{
	SHELL_REGISTER("lora", commands);

	uart = device_get_binding(CONFIG_LORA_IM881A_UART_DRV_NAME);
	uart_cfg = (struct uart_device_config *)(uart->config->config_info);

	/* BUG: couldn't launch the debug when the function 'uart_irq_rx_enable' is called tool quickly */
	k_busy_wait(1000000);

	wimod_lorawan_init();

	wimod_lorawan_join_network_request(join_callback);

	disable_uart();

}
