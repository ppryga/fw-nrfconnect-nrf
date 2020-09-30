/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <string.h>

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/printk.h>
#include <bluetooth/dfe_data.h>

#include "if.h"
#include "protocol.h"
#include "dfe_local_config.h"
#include "dfe_data_preprocess.h"
#include "ble.h"

#include "aoa.h"
#include "if.h"
#include "ble.h"
#include "protocol.h"

#include "average_results.h"
#include "float_ring_buffer.h"

/** @brief Number of ms to wait for a data before printing no data note
 */
#define WAIT_FOR_DATA_BEFORE_PRINT (1000)

/** @brief Antennas are placed around center and create square,
 * this is number of antennas on single edge of matrix.
 */
#define AOA_MATRIX_SIZE		4
#define ANTENA_DISTANCE 	(0.05f)
#define BLE_WAVE_LEN		(0.125f)
#define AOA_DISTANCE		(ANTENA_DISTANCE/BLE_WAVE_LEN)
#define AOA_FREQUENCY		250000
#define AOA_DEGTORAD		(PI/180)

/** @brief Instance of a data structure to provide access to system calls
 * from aoa_library.
 */
static const struct aoa_system_interface sys_iface =
{
	.uptime_get = k_uptime_get,
};

/** @brief Queue defined by BLE Controller to provide IQ samples data
 */
extern struct k_msgq df_packet_msgq;

/** @brief Variable to store handle received from aoa_library initialization
 */
static void* handle;

/** @brief Variable to store results of AoA evaluation
 */
static struct aoa_results results = {0};
/** @brief Variable to store filtered results of AoA evaluation
 */
static struct aoa_results avg_results;

/** buffer for IQ samples */
static struct dfe_iq_data_storage iq_storage = {
	.slots_num = DFE_TOTAL_SLOTS_NUM,
	.samples_num = DFE_SAMPLES_PER_SLOT_NUM,
};

static struct dfe_slot_samples_storage slots_storage = {
		.slots_num = DFE_TOTAL_SLOTS_NUM,
};

/** @brief Main function of the example.
 *
 * The function is responsible for:
 * - initialization of UART output
 * - initialization of Direction Finding in Bluetooth stack
 * - initialization of Bluetooth stack
 * - initialization of angle of arrival library
 * - receive DFE data from Bluetooth controller
 * - mapping received data to antenna numbers
 * - store raw IQ samples in a transfer buffer
 * - remove IQ samples gathered during antenna switchign
 * - evaluate angle of arrival
 * - filter evaluated angles
 * - store angles in transfer data
 * - forwarding data by UART
 *
 * Steps between receive and forward data are processed in infinite loop.
 */
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
		.sampling_slots_num = dfe_get_sampling_slots_num(sampl_conf),
		.reference_period = sampl_conf->ref_period_us,
		.ant_switch_spacing = dfe_get_switch_spacing_ns(sampl_conf->switch_spacing) / DFE_NS(1000),
		.sample_spacing_ref = dfe_get_sample_spacing_ref_ns(sampl_conf->sample_spacing_ref),
		.sample_spacing = dfe_get_sample_spacing_ns(sampl_conf->sample_spacing),
		.slot_samples_num = dfe_get_sampling_slot_samples_num(sampl_conf),
		.frequency = AOA_FREQUENCY,
		.array_distance = AOA_DISTANCE,
		.pdda_coarse_step = CONFIG_AOA_LOCATOR_PDDA_COARSE_STEP,
		.pdda_fine_step   = CONFIG_AOA_LOCATOR_PDDA_FINE_STEP
	};

	if (!aoa_initialize(&sys_iface, &aoa_config, &handle)) {
		printk("Aoa initialized\r\n");
	}

	while(1)
	{
		static struct dfe_packet df_data_packet;
		static struct dfe_mapped_packet df_data_mapped;

		//static struct dfe_mapped_packet df_data_cleanet;

		memset(&df_data_packet, 0, sizeof(df_data_packet));
		memset(&df_data_mapped, 0, sizeof(df_data_mapped));
		//memset(&df_data_cleanet, 0, sizeof(df_data_cleanet));
		k_msgq_get(&df_packet_msgq, &df_data_packet, K_MSEC(WAIT_FOR_DATA_BEFORE_PRINT));
		if (df_data_packet.hdr.length != 0) {
			printk("\r\nData arrived...\r\n");

			dfe_map_iq_samples_to_antennas(&df_data_mapped,
										   &iq_storage,
										   &slots_storage,
										   &df_data_packet,
										   sampl_conf, ant_conf);

			data_tranfer_prepare_header();
			data_transfer_prepare_samples(sampl_conf,&df_data_mapped);

			remove_samples_from_switch_slot(&df_data_mapped, sampl_conf);
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

			data_tranfer_prepare_results(sampl_conf, &results);
			data_tranfer_prepare_footer();
			data_tranfer_send();
		}
		else
		{
			printk("\r\nNo data received.");
		}
		k_sleep(K_MSEC(CONFIG_AOA_LOCATOR_DATA_SEND_WAIT_MS));
	}
}
