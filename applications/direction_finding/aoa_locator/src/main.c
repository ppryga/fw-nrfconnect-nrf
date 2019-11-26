#include <logging/log.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <misc/util.h>
#include <device.h>
#include <init.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "aoa.h"
#include "if.h"
#include "ble.h"
#include "protocol.h"
#include "ll_sw/df_config.h"

static const aoa_system_interface sys_iface =
{
	.uptime_get = k_uptime_get,
	.get_sample_antenna_ids = df_get_sample_antenna_ids,
	.protocol = &PROTOCOL,
};

extern struct k_msgq df_packet_msgq;
extern if_vector IF;

void main(void)
{
	if_vector* iface = IF_Initialization();
	
	PROTOCOL_Initialization(iface);
	BLE_Initialization();
	if (!AOA_Initialization(&sys_iface)) {
		printk("Aoa initialized\r\n");
	}
	k_sleep(K_MSEC(100));
	while(1)
	{
		struct df_packet df_packet = {0};
		memset(&df_packet, 0, sizeof(df_packet));
		k_msgq_get(&df_packet_msgq, &df_packet, K_NO_WAIT);
		if (df_packet.hdr.length != 0) {
			//printk("data arrived\r\n");
			AOA_Handling(&df_packet);
			PROTOCOL_Handling();
		}
		// else {
		// 	printk("no data\r\n");
		// }
	 	k_sleep(K_MSEC(1));
	}
}
