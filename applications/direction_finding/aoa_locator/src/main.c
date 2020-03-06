/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <string.h>

#include <kernel.h>
#include <zephyr/types.h>
#include <misc/printk.h>
#include <bluetooth/dfe_data.h>

#include "if.h"
#include "protocol.h"
#include "dfe_local_config.h"
#include "ble.h"

#include "aoa.h"
#include "if.h"
#include "ble.h"
#include "protocol.h"

#include "average_results.h"
#include "float_ring_buffer.h"

/*number of loops to to avid when printing */
#define MAIN_LOOP_NO_MSG_COUNT (1000)

/*
 * Antennas are placed around center and create square,
 * this is number of antennas on single edge of matrix.
 */
#define AOA_MATRIX_SIZE		4
#define ANTENA_DISTANCE 	(0.05f)
#define BLE_WAVE_LEN		(0.125f)
#define AOA_DISTANCE		(ANTENA_DISTANCE/BLE_WAVE_LEN)
#define AOA_FREQUENCY		250000
#define AOA_DEGTORAD		(PI/180)

static const struct aoa_system_interface sys_iface =
{
	.uptime_get = k_uptime_get,
};

extern struct k_msgq df_packet_msgq;
static void* handle;
static struct aoa_results results = {0};
static struct aoa_results avg_results;

void main(void)
{
	printk("Starting AoA Locator CL!\r\n");
	/* initialize UART interface to provide I/Q samples */
	struct if_data* iface = if_initialization();

	if (iface == NULL) {
		printk("Locator stopped!\r\n");
		return;
	}

	int err;

	err = data_transfer_init(iface);
	if (err) {
		printk("Locator stopped!\r\n");
		return;
	}

	const struct dfe_sampling_config* sampl_conf = NULL;
	const struct dfe_antenna_config* ant_conf = NULL;
	const struct dfe_ant_gpio* ant_gpio = NULL;
	u8_t ant_gpio_len = 0;

	sampl_conf = dfe_get_sampling_config();
	ant_conf = dfe_get_antenna_config();
	ant_gpio_len = dfe_get_ant_gpios_config_len();
	ant_gpio = dfe_get_ant_gpios_config();

	assert(sampl_conf != NULL);
	assert(ant_conf != NULL);
	assert(ant_gpio != NULL);
	assert(ant_gpio_len != 0);

	printk("Initialize DFE\r\n");
	err = dfe_init(sampl_conf, ant_conf, ant_gpio, ant_gpio_len);
	if (err) {
		printk("Locator stopped!\r\n");
		return;
	}

	printk("Initialize Bluetooth\r\n");
	ble_initialization();

	struct aoa_configuration aoa_config = {
		.matrix_size = AOA_MATRIX_SIZE,
		//.antennas_num = ant_config->antennae_switch_idx_len,
		.antennas_num = dfe_get_effective_ant_num(sampl_conf),
		.reference_period = sampl_conf->ref_period_us,
		.ant_switch_spacing = dfe_get_switch_spacing_ns(sampl_conf->switch_spacing) / K_NSEC(1000),
		.sample_spacing_ref = dfe_get_sample_spacing_ref_ns(sampl_conf->sample_spacing_ref),
		.sample_spacing = dfe_get_sample_spacing_ns(sampl_conf->sample_spacing),
		.slot_samples_num = dfe_get_sampling_slot_samples_num(sampl_conf),
		.frequency = AOA_FREQUENCY,
		.array_distance = AOA_DISTANCE,
	};

	if (!aoa_initialize(&sys_iface, &aoa_config, &handle)) {
		printk("Aoa initialized\r\n");
	}

	while(1)
	{
		static u16_t no_msg_counter = 0;
		static struct df_packet df_data_packet;
		static struct dfe_mapped_packet df_data_mapped;
		//static struct dfe_mapped_packet df_data_cleanet;

		memset(&df_data_packet, 0, sizeof(df_data_packet));
		memset(&df_data_mapped, 0, sizeof(df_data_mapped));
		//memset(&df_data_cleanet, 0, sizeof(df_data_cleanet));
		k_msgq_get(&df_packet_msgq, &df_data_packet, K_NO_WAIT);
		if (df_data_packet.hdr.length != 0) {
			printk("\r\nData arrived...\r\n");

			dfe_map_iq_samples_to_antennas(&df_data_mapped,
						      &df_data_packet,
						      sampl_conf, ant_conf);

			data_tranfer_prepare_header();
			data_transfer_prepare_samples(sampl_conf,&df_data_mapped);

			remove_samples_from_switch_slot(/*&df_data_cleanet,*/
							&df_data_mapped,
							sampl_conf);
			int err = aoa_handling(handle, &df_data_mapped, &results);
			if (err) {
				printk("AoA_Handling error: %d! Stopping the evaluation.\r\n", err);
				break;
			}
			err = low_pass_filter_FIR(&results, &avg_results);
			if (err) {
				printk("Averaging error: %d\r\n", err);
				break;
			}
			results.filtered_result.azimuth = avg_results.raw_result.azimuth;
			results.filtered_result.elevation = avg_results.raw_result.elevation;

//			err = protocol_handling(sampl_conf, &df_data_mapped, &results);
//			if (err) {
//				printk("Error in protocol handling!\r\n");
//				printk("Locator stopped!\r\n");
//				return;
//			}

			data_tranfer_prepare_results(sampl_conf, &results);
			data_tranfer_prepare_footer();
			data_tranfer_send();

			no_msg_counter = 0;
		}
		else
		{
			if (no_msg_counter > 0) {
				--no_msg_counter;
			} else {
				printk("\r\nNo data received.");
				no_msg_counter = MAIN_LOOP_NO_MSG_COUNT;
			}
		}
		k_sleep(K_MSEC(1));
	}
}
