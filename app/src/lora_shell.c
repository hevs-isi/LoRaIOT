#include <zephyr.h>
#include <shell/shell.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(lora_shell, LOG_LEVEL_DBG);

#ifdef CONFIG_LORA_IM881A
#include <wimod_lorawan_api.h>
#endif

#include "lora.h"

static int shell_cmd_info(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_firmware_version();

	return 0;
}

static int shell_cmd_custom(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_op_mode();

	return 0;
}

static int shell_cmd_getmode(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_op_mode();

	return 0;
}

static int shell_cmd_get_rtc(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rtc();

	return 0;
}

static int shell_cmd_set_rtc(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_rtc();

	return 0;
}

static int shell_cmd_get_rtc_alarm(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rtc_alarm();

	return 0;
}

static int shell_factory_reset(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_factory_reset();

	return 0;
}

static int shell_cmd_deveui(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_device_eui();

	return 0;
}

static int shell_cmd_setparam(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_join_param_request();

	return 0;
}

static int shell_cmd_join(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_join_network_request(NULL);

	return 0;
}

static int shell_nwk_status(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_nwk_status();

	return 0;
}

static int shell_set_rstack_cfg(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_set_rstack_config();

	return 0;
}

static int shell_rstack_cfg(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	wimod_lorawan_get_rstack_config();

	return 0;
}

static int shell_send_udata(const struct shell *shell, size_t argc, char *argv[])
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

static int shell_send_cdata(const struct shell *shell, size_t argc, char *argv[])
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

SHELL_CREATE_STATIC_SUBCMD_SET(lora_sub)
{
	SHELL_CMD_ARG(firmware, NULL, "no help", shell_cmd_info, 0, 1),
	SHELL_CMD_ARG(custom, NULL, "no help", shell_cmd_custom, 0, 1),
	SHELL_CMD_ARG(getmode, NULL, "no help", shell_cmd_getmode, 0, 1),
	SHELL_CMD_ARG(get_rtc, NULL, "no help", shell_cmd_get_rtc, 0, 1),
	SHELL_CMD_ARG(set_rtc, NULL, "no help", shell_cmd_set_rtc, 0, 1),
	SHELL_CMD_ARG(get_rtc_alarm, NULL, "no help", shell_cmd_get_rtc_alarm, 0, 1),
	SHELL_CMD_ARG(factory, NULL, "no help", shell_factory_reset, 0, 1),
	SHELL_CMD_ARG(deveui, NULL, "no help", shell_cmd_deveui, 0, 1),
	SHELL_CMD_ARG(set, NULL, "no help", shell_cmd_setparam, 0, 1),
	SHELL_CMD_ARG(join, NULL, "no help", shell_cmd_join, 0, 1),
	SHELL_CMD_ARG(nwk, NULL, "no help", shell_nwk_status, 0, 1),
	SHELL_CMD_ARG(setrstack, NULL, "no help", shell_set_rstack_cfg, 0, 1),
	SHELL_CMD_ARG(rstack, NULL, "no help", shell_rstack_cfg, 0, 1),
	SHELL_CMD_ARG(set_rstack, NULL, "no help", shell_set_rstack_cfg, 0, 1),
	SHELL_CMD_ARG(udata, NULL, "no help", shell_send_udata, 0, 1),
	SHELL_CMD_ARG(cdata, NULL, "no help", shell_send_cdata, 0, 1),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(lora, &lora_sub, "LoRa commands", NULL);
