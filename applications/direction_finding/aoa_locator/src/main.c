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

#define AOA_ANTENNAS_NUM	14	// number of antennas on board
#define AOA_MATRIX_SIZE		4	// antennas are placed around center and create square - this is the size of the
#define AOA_SNAPSHOT_LENGTH	1	// once cycle through antennas array (?)
#define AOA_DISTANCE		0.4f	// distance between neighbor antennas
#define AOA_REF_PERIOD		8	// number of "sampling time slots" in reference signal acquisition

#define AOA_FREQUENCY		250000
#define AOA_DEGTORAD		(PI/180)

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

	df_antenna_config* ant_config = df_get_antenna_config();
	df_sampling_config *sampling_config = df_get_sampling_config();

	uint8_t antennas_num = df_get_effective_ant_num(df_get_number_of_8us(), ant_config->switch_spacing, sampling_config);

	aoa_configuration aoa_config = {
			.matrix_size = AOA_MATRIX_SIZE,
			//.antennas_num = ant_config->antennae_switch_idx_len,
			.antennas_num = antennas_num,
			.snapshot_len = AOA_SNAPSHOT_LENGTH,
			.reference_period = sampling_config->ref_period_us,
			.df_sw = df_get_switch_spacing_ns(ant_config->switch_spacing)/K_NSEC(1000),
			.df_r = df_get_sample_spacing_ref_ns(sampling_config->sample_spacing_ref),
			.df_s = df_get_sample_spacing_ns(sampling_config->sample_spacing),
			.frequency = AOA_FREQUENCY,
			.array_distance = AOA_DISTANCE,
	};

	if (!AOA_Initialize(&sys_iface, &aoa_config, &handle)) {
		printk("Aoa initialized\r\n");
	}

	k_sleep(K_MSEC(100));

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
			PROTOCOL_Handling(&aoa_config, &df_packet, &results);
		}
		else
		{
			printk("no data\r\n");
		}
	 	k_sleep(K_MSEC(1));
	}
}
