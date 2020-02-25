/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <misc/printk.h>
#include <misc/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/dfe_config.h>

#define BT_DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define BT_DEVICE_NAME_LEN (sizeof(BT_DEVICE_NAME) - 1)
/* Advertising interval provided in number of 0.625ms.
 * In case of the example 160 * 0.625ms = 100ms.
 */
#define BT_ADV_INTERVAL (160)
/* duration of DFE in number of 8us (e.g. 10*8us = 80us).
 * DFE is a Direction Finding Extension and may be used interchangeably
 * with CTE - Constant Tone Extension. The first one is used by Nordics radio
 * configuration registers, the second one is used by BT specification. */
#define BT_DFE_DURATION (10)

/* Set Scan Response data */
const static struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, BT_DEVICE_NAME, BT_DEVICE_NAME_LEN),
};

static bt_addr_le_t addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {
		.val = {0x01,0x02,0x03,0x04,0x05,0xc6},
	},
};

struct dfe_beacon_config {
	u8_t dfe_duration;
	u8_t dfe_mode;
};

const static struct dfe_beacon_config dfe_config = {
	.dfe_duration = BT_DFE_DURATION,
	.dfe_mode = RADIO_DFEMODE_DFEOPMODE_AoA,
};

static void bt_ready(int err)
{
	if (err) {
		printk("[BT] - Callback initialization failed (err %d)\n", err);
		return;
	}

	printk("[BT] - Initialization finished\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_PARAM(BT_LE_ADV_OPT_USE_IDENTITY,BT_ADV_INTERVAL,BT_ADV_INTERVAL),
			      NULL, 0, sd, ARRAY_SIZE(sd));
	if (err) {
		printk("[BT] - Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Beacon started\n");
}

static int bt_init()
{
	int err;
	BT_ADDR_SET_STATIC(&addr.a);
	err = bt_set_id_addr(&addr);
	if (err) {
		printk("[BT] - MAC setting error %d\n",err);
		return err;
	}
	err = bt_enable(bt_ready);
	if (err) {
		printk("[BT] - Initialization failed (err %d)\n", err);
		return err;
	}
	return 0;
}

static int dfe_init()
{
	int err;
	err = dfe_set_mode(dfe_config.dfe_mode);
	if (err) {
		printk("[DFE] - DFE mode is unknown\n");
		return -EINVAL;
	}

	err = dfe_set_duration(dfe_config.dfe_duration);
	if (err) {
		printk("[DFE] - DFE duration value out of range\n");
		return -EINVAL;
	}
	return 0;
}

void main(void)
{
	int err;
	printk("Starting Beacon\n");

	/* Set DFE duration */
	err = dfe_init();
	if (err) {
		printk("Beacon stopped - Error\n");
		return;
	}

	err = bt_init();
	if (err) {
		printk("Beacon stopped - Error\n");
		return;
	}

	while(1) {
		printk("Running\r\n");
	}
}
