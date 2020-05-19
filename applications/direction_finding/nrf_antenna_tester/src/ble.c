/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <misc/printk.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "ble.h"

#define MODULE df_ble
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DF_APP_LOG_LEVEL);

int ble_initialization(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
		.interval   = 0x0020,
		.window     = 0x0020,
	};
	int err;

	LOG_INF("Initialization started");

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Initialization failed (err %d)", err);
		return err;
	}

	LOG_INF("Starting scanning");
	err = bt_le_scan_start(&scan_param, NULL);
	if (err)
	{
		LOG_ERR("Start scanning failed (err %d)", err);
		return err;
	}
	return 0;
}
