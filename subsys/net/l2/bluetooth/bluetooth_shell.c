/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_bt_shell, CONFIG_NET_L2_BT_LOG_LEVEL);

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <string.h>
#include <errno.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/bt.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

static int char2hex(const char *c, u8_t *x)
{
	if (*c >= '0' && *c <= '9') {
		*x = *c - '0';
	} else if (*c >= 'a' && *c <= 'f') {
		*x = *c - 'a' + 10;
	} else if (*c >= 'A' && *c <= 'F') {
		*x = *c - 'A' + 10;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int str2bt_addr_le(const char *str, const char *type, bt_addr_le_t *addr)
{
	int i, j;
	u8_t tmp;

	if (strlen(str) != 17) {
		return -EINVAL;
	}

	for (i = 5, j = 1; *str != '\0'; str++, j++) {
		if (!(j % 3) && (*str != ':')) {
			return -EINVAL;
		} else if (*str == ':') {
			i--;
			continue;
		}

		addr->a.val[i] = addr->a.val[i] << 4;

		if (char2hex(str, &tmp) < 0) {
			return -EINVAL;
		}

		addr->a.val[i] |= tmp;
	}

	if (!strcmp(type, "public") || !strcmp(type, "(public)")) {
		addr->type = BT_ADDR_LE_PUBLIC;
	} else if (!strcmp(type, "random") || !strcmp(type, "(random)")) {
		addr->type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int shell_cmd_connect(const struct shell *shell,
			     size_t argc, char *argv[])
{
	int err;
	bt_addr_le_t addr;
	struct net_if *iface = net_if_get_default();

	if (argc < 3) {
		shell_help(shell);
		return -ENOEXEC;
	}

	err = str2bt_addr_le(argv[1], argv[2], &addr);
	if (err) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Invalid peer address (err %d)\n", err);
		return 0;
	}

	if (net_mgmt(NET_REQUEST_BT_CONNECT, iface, &addr, sizeof(addr))) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Connection failed\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Connection pending\n");
	}

	return 0;
}

static int shell_cmd_scan(const struct shell *shell,
			  size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (argc < 2) {
		shell_help(shell);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_BT_SCAN, iface, argv[1], strlen(argv[1]))) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Scan failed\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Scan in progress\n");
	}

	return 0;
}

static int shell_cmd_disconnect(const struct shell *shell,
				size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (net_mgmt(NET_REQUEST_BT_DISCONNECT, iface, NULL, 0)) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Disconnect failed\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Disconnected\n");
	}

	return 0;
}

static int shell_cmd_advertise(const struct shell *shell,
			       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (argc < 2) {
		shell_help(shell);
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_BT_ADVERTISE, iface, argv[1],
		     strlen(argv[1]))) {
		shell_fprintf(shell, SHELL_WARNING,
			      "Advertise failed\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Advertise in progress\n");
	}

	return 0;
}

SHELL_CREATE_STATIC_SUBCMD_SET(bt_commands)
{
	SHELL_CMD(advertise, NULL,
		  "on/off",
		  shell_cmd_advertise),
	SHELL_CMD(connect, NULL,
		  "<address: XX:XX:XX:XX:XX:XX> <type: (public|random)>",
		  shell_cmd_connect),
	SHELL_CMD(scan, NULL,
		  "<on/off/active/passive>",
		  shell_cmd_scan),
	SHELL_CMD(disconnect, NULL,
		  "",
		  shell_cmd_disconnect),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(net_bt, &bt_commands, "Net Bluetooth commands", NULL);

void net_bt_shell_init(void)
{
}
