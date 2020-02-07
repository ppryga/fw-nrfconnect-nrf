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
#include "ll_sw/df_data.h"
#include "average_results.h"
#include "float_ring_buffer.h"

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

int df_map_iq_samples_to_antennas(struct df_packet *raw_data,
				  struct df_packet_ex *mapped_data,
				  struct df_sampling_config *sampling_conf,
				  struct df_antenna_config *ant_config)
{
	assert(raw_data != NULL);
	assert(mapped_data != NULL);

	u16_t ref_samples_num = df_get_ref_samples_num(sampling_conf);
	mapped_data->ref_data.antenna_id = ant_config->ref_ant_idx;

	for(uint16_t idx = 0; idx < ref_samples_num; ++idx) {
		mapped_data->ref_data.data[idx].i = raw_data->data[idx].iq.i;
		mapped_data->ref_data.data[idx].q = raw_data->data[idx].iq.q;
	}
	mapped_data->ref_data.samples_num = ref_samples_num;

	u16_t effective_ant_num = df_get_effective_ant_num(df_get_number_of_8us(),
							   ant_config->switch_spacing,
							   sampling_conf);

	u8_t samples_num = df_get_sampling_slot_samples_num(ant_config, sampling_conf);
	u8_t switch_period_sampl_num = df_get_switch_period_samples_num(ant_config, sampling_conf);

	u8_t effective_sample_idx;

	for(u16_t ant_idx = 0; ant_idx < effective_ant_num; ++ant_idx) {
		mapped_data->sampl_data[ant_idx].antenna_id = ant_config->antennae_switch_idx[ant_idx % ant_config->antennae_switch_idx_len];
		for(u8_t sample_idx = 0; sample_idx < samples_num; ++sample_idx) {
			effective_sample_idx = ref_samples_num + (ant_idx * switch_period_sampl_num) + sample_idx;
			mapped_data->sampl_data[ant_idx].data[sample_idx].i = raw_data->data[effective_sample_idx].iq.i;
			mapped_data->sampl_data[ant_idx].data[sample_idx].q = raw_data->data[effective_sample_idx].iq.q;
		}
		mapped_data->sampl_data[ant_idx].samples_num = samples_num;
	}

	mapped_data->header.length = effective_ant_num;
	mapped_data->header.frequency = raw_data->hdr.frequency;
	return 0;
}

void main(void)
{
	struct if_data* iface = IF_Initialization();

	if (iface == NULL) {
		printk("Output interface initialization failed! Terminating!\r\n");
		return;
	}

	if (PROTOCOL_Initialization(iface)) {
		printk("Protocol intialization failed! Terminating!\r\n");
		return;
	}

	BLE_Initialization();

	struct df_antenna_config* ant_config = df_get_antenna_config();
	struct df_sampling_config *sampling_config = df_get_sampling_config();

	uint8_t antennas_num = df_get_effective_ant_num(df_get_number_of_8us(), ant_config->switch_spacing, sampling_config);
	uint16_t sample_spacing_ns = df_get_sample_spacing_ns(sampling_config->sample_spacing);
	uint16_t slot_samples_num = df_get_switch_spacing_ns(ant_config->switch_spacing) / (sample_spacing_ns * 2);
	struct aoa_configuration aoa_config = {
			.matrix_size = AOA_MATRIX_SIZE,
			//.antennas_num = ant_config->antennae_switch_idx_len,
			.antennas_num = antennas_num,
			.reference_period = sampling_config->ref_period_us,
			.df_sw = df_get_switch_spacing_ns(ant_config->switch_spacing)/K_NSEC(1000),
			.df_r = df_get_sample_spacing_ref_ns(sampling_config->sample_spacing_ref),
			.df_s = sample_spacing_ns,
			.slot_samples_num = slot_samples_num,
			.frequency = AOA_FREQUENCY,
			.array_distance = AOA_DISTANCE,
	};

	if (!AOA_Initialize(&sys_iface, &aoa_config, &handle)) {
		printk("Aoa initialized\r\n");
	}

	k_sleep(K_MSEC(100));

	while(1)
	{
		struct df_packet df_data_packet = {0};
		memset(&df_data_packet, 0, sizeof(struct df_packet));
		k_msgq_get(&df_packet_msgq, &df_data_packet, K_NO_WAIT);
		if (df_data_packet.hdr.length != 0) {
			printk("data arrived\r\n");
			struct df_packet_ex df_data_mapped = {0};
			df_map_iq_samples_to_antennas(&df_data_packet, &df_data_mapped,
						      df_get_sampling_config(),
						      df_get_antenna_config());
			int err = AOA_Handling(handle, &df_data_mapped, &results);
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
			PROTOCOL_Handling(&aoa_config, &df_data_packet, &results);
		}
		else
		{
			printk("no data\r\n");
		}
	 	k_sleep(K_MSEC(1));
	}
}
