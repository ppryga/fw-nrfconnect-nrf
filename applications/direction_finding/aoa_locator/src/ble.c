#include <stddef.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "ble.h"


/*static u8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data, 3),
};*/

static void scan_cb(const bt_addr_le_t *addr, s8_t rssi, u8_t adv_type,
		    struct net_buf_simple *buf)
{
	/*u8_t target[6] = {0x01,0x02,0x03,0x04,0x05,0xc6};
	if(memcmp(addr->a.val,target,6) == 0) {
		printk("-----------------------------\n");
	}*/
	/*printk("addr: %x:%x:%x:%x:%x:%x\n", addr->a.val[5],
										addr->a.val[4],
										addr->a.val[3],
										addr->a.val[2],
										addr->a.val[1],
										addr->a.val[0]);
	*/
	//mfg_data[2]++;
}


void BLE_Initialization(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
		.interval   = 0x0020,
		.window     = 0x0020,
	};
	int err;

	printk("Starting Scanner/Advertiser Demo\n");

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	printk("Bluetooth initialized\n");

	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err)
	{
		printk("Starting scanning failed (err %d)\n", err);
		return;
	}
}
