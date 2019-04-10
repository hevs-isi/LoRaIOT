#include <zephyr.h>
#include <shell/shell.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(shell_power, LOG_LEVEL_INF);

#include <shell/shell_log_backend.h>
#include <shell/shell_uart.h>
#include "stm32_lp.h"

static int shell_sleep(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	char *endptr;
	u32_t duration;

	if (argc != 2)
	{
		shell_error(shell, "arguments?");
		return -EINVAL;
	}
	duration = strtol(argv[1], &endptr, 0);

	if (endptr == argv[1])
	{
		shell_error(shell, "error parsing  argv[1] : '%s'", argv[1]);
		return -EINVAL;
	}

	k_sleep(100);

	stm32_sleep(duration);

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(power_sub)
{
	SHELL_CMD_ARG(sleep, NULL, "no help", shell_sleep, 0, 0),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(power, &power_sub, "power commands", NULL);
