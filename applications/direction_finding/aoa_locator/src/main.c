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
#include "average_results.h"
#include "float_ring_buffer.h"

static const aoa_system_interface sys_iface =
{
	.uptime_get = k_uptime_get,
	.get_sample_antenna_ids = df_get_sample_antenna_ids,
};

extern struct k_msgq df_packet_msgq;
static void* handle;
static aoa_results results = {0,0};
static aoa_results avg_results;

void main(void)
{
	if_data* iface = IF_Initialization();

	if (iface == NULL) {
		printk("Output interface initialization failed! Terminating!\r\n");
		return;
	}

	if (PROTOCOL_Initialization(iface)) {
		printk("Protocol intialization failed! Terminating!\r\n");
		return;
	}

	BLE_Initialization();
	if (!AOA_Initialize(&sys_iface, &handle)) {
		printk("Aoa initialized\r\n");
	}

	k_sleep(K_MSEC(100));

	const aoa_configuration* aoa_config = AOA_GetConfiguration(handle);

	while(1)
	{
		struct df_packet df_packet = {0};
		memset(&df_packet, 0, sizeof(df_packet));
		k_msgq_get(&df_packet_msgq, &df_packet, K_NO_WAIT);
		if (df_packet.hdr.length != 0) {
			printk("data arrived\r\n");
			int err = AOA_Handling(handle, &df_packet, &results);
			if (err) {
				printk("AoA_Handling error: %d! Stopping the evaluation.\r\n", err);
				break;
			}
			err = LowPassFilter_FIR(&results, &avg_results);
			if (err) {
				printk("Averaging error: %d\r\n", err);
				break;
			}
			results.filtered_result.azimuth = avg_results.raw_result.azimuth;
			results.filtered_result.elevation = avg_results.raw_result.elevation;
			PROTOCOL_Handling(aoa_config, &df_packet, &results);
		}
		else
		{
			printk("no data\r\n");
		}
	 	k_sleep(K_MSEC(1));
	}
}
