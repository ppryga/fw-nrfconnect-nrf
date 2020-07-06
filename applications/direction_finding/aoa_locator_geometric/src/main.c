/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <string.h>
#include <complex.h>
#include <math.h>

#include <kernel.h>
#include <zephyr/types.h>
#include <sys/printk.h>
#include <bluetooth/dfe_data.h>

#include "if.h"
#include "protocol.h"
#include "dfe_local_config.h"
#include "ble.h"
#include "dfresults.h"


/** @brief Number of ms to wait for a data before printing no data note
 */
#define WAIT_FOR_DATA_BEFORE_PRINT (1000)

/** @brief Queue defined by BLE Controller to provide IQ samples data
 */
extern struct k_msgq df_packet_msgq;
/** @brief Compute average phase difference between samples in reference
 *
 * The function computes average difference between samples from reference period.
 *
 * @param ref_data Reference data samples
 *
 * @return The average phase offset between samples
 */
static float compute_phase_avg_diffrence(const struct dfe_ref_samples *ref_data)
{
	assert(ref_data != NULL);

	uint16_t samples_num = ref_data->samples_num;
	float complex z1 = 0, z2 = 0;
	float sum= 0.0;

	for(uint16_t idx=0; idx<samples_num-1; ++idx)
	{
		float temp;
		z1 = ref_data->data[idx].i   + ref_data->data[idx].q*I;
		z2 = ref_data->data[idx+1].i + ref_data->data[idx+1].q*I;
		temp = cargf(z1/z2);

		sum += temp;
	}

	return sum / (samples_num - 1);
}

/** @brief Function that shifts phase of I/Q samples back into one point in time
 *
 * The samples are sampled one after another.
 * The mathematical model used to calculate the AoA requires all the samples
 * to be taken in single moment in time.
 * The function here calculates phase shift between samples in reference period
 * and uses it to correct the phase for the samples in reference and sampling
 * periods in such a way like they were all taken in the same time.
 *
 * @param[in,out]  data		Pointer to I/Q samples. After end of evaluation
 * 				the data are overwritten with corrected values.
 * @param[in]      slot_samples_num Number of samples in single slot.
 */
static void phase_time_machine(struct dfe_mapped_packet *data,
			       uint16_t slot_samples_num)
{
	float complex phase_correction;
	float complex source_phase, out_phase;
	float phase_diff = compute_phase_avg_diffrence(&data->ref_data);

	/* correct phase in ref. period*/
	for(uint16_t idx=0; idx<data->ref_data.samples_num; ++idx)
	{
		source_phase = data->ref_data.data[idx].i + data->ref_data.data[idx].q*I;
		float total_fix = phase_diff * (float)idx;
		phase_correction = cosf(total_fix) + sinf(total_fix)*I;
		out_phase = source_phase * phase_correction;
		data->ref_data.data[idx].i = crealf(out_phase);
		data->ref_data.data[idx].q = cimagf(out_phase);
	}

	/* correct phase in switching period.
	 * Pay attention that sampling is started just after first switch slot,
	 * that means there is additional time delay of TSWITCHING/2 */
	uint16_t time_delay = data->ref_data.samples_num + slot_samples_num;
	uint16_t switch_sampl_num = slot_samples_num * 2;
	uint16_t sample_effective_delay;
	float phase_effective_offset;

	for( uint16_t ant_idx = 0; ant_idx<data->header.length; ++ant_idx) {
		for(uint16_t idx = 0; idx < data->sampl_data[ant_idx].samples_num; ++idx) {
			source_phase = data->sampl_data[ant_idx].data[idx].i +
				       data->sampl_data[ant_idx].data[idx].q*I;

			sample_effective_delay = time_delay + idx + (ant_idx * switch_sampl_num);
			phase_effective_offset = phase_diff * sample_effective_delay;
			phase_correction = cosf(phase_effective_offset) + sinf(phase_effective_offset)*I;
			out_phase = source_phase * phase_correction;
			data->sampl_data[ant_idx].data[idx].i = crealf(out_phase);
			data->sampl_data[ant_idx].data[idx].q = cimagf(out_phase);
		}
	}
}

/**
 * @brief Calculate sum of the samples
 *
 * @param sampl_data Data chunk to calculate sum of
 *
 * @return The sum of the samples in data chunk
 */
static float complex antenna_samples_sum(const struct dfe_samples* sampl_data)
{
	float complex sum = 0;

	for (u16_t i = 0; i < sampl_data->samples_num; ++i)
	{
		sum += sampl_data->data[i].i + sampl_data->data[i].q*I;
	}
	return sum;
}

/**
 * @brief Calculate phase difference between antennas
 *
 * The selected antennas phases are averaged and then the wave angle is calculated.
 *
 * @note This function expects the angles to be corrected to frequency 0 (DC).
 *
 * @param mapped_data The data do process
 * @param a1          First antenna number
 * @param a2          Second antenna number
 *
 * @return The phase difference between phases on antennas 1 and 2.
 */
static float antenna_data_to_phase_diff(const struct dfe_mapped_packet *mapped_data, u8_t a1, u8_t a2)
{
	float complex sum1 = 0, sum2 = 0;

	for (u16_t idx=0; idx<mapped_data->header.length; ++idx)
	{
		const struct dfe_samples* sampl_data = &mapped_data->sampl_data[idx];
		if (sampl_data->antenna_id == a1)
			sum1 += antenna_samples_sum(sampl_data);
		if (sampl_data->antenna_id == a2)
			sum2 += antenna_samples_sum(sampl_data);
	}
	return cargf(sum1/sum2);
}

/**
 * @brief Calculate the phase difference into angle in radians
 *
 * @param phase Phase difference between antennas
 * @param d     Distance between antennas
 * @param freq  Radio frequency to calculate wavelength
 *
 * @return The calculated angle
 */
static float phase_to_angle(float phase, float d, float freq)
{
	float arg = (phase * (float)WAVE_SPEED) / (2 * (float)PI * freq * d);

	if (arg > 1.0f)
		arg = 1.0f;
	else if (arg < -1.0f)
		arg = -1.0f;
	return acos(arg);
}

/** @brief Main function of the example.
 *
 * The function is responsible for:
 * - initialization of UART output
 * - initialization of Direction Finding in Bluetooth stack
 * - initialization of Bluetooth stack
 * - receive DFE data from Bluetooth controller
 * - mapping received data to antenna numbers
 * - forwarding data by UART
 *
 * Following steps: data receive, mapping and their forwarding
 * is done in never ending loop.
 */
void main(void)
{
	printk("Starting AoA Locator Geometric!\r\n");
	/* initialize UART interface to provide I/Q samples */
	struct if_data* iface = if_initialization();

	if (iface == NULL) {
		printk("Locator stopped!\r\n");
		return;
	}

	int err;

	err = protocol_initialization(iface);
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

	while(1)
	{
		static struct dfe_packet df_data_packet;
		static struct dfe_mapped_packet df_data_mapped;
		static struct dfresults results = {0};

		memset(&df_data_packet, 0, sizeof(df_data_packet));
		memset(&df_data_mapped, 0, sizeof(df_data_mapped));
		k_msgq_get(&df_packet_msgq, &df_data_packet, K_MSEC(WAIT_FOR_DATA_BEFORE_PRINT));
		if (df_data_packet.hdr.length != 0) {
			dfe_map_iq_samples_to_antennas(&df_data_mapped,
						       &df_data_packet,
						       sampl_conf, ant_conf);
			remove_samples_from_switch_slot(&df_data_mapped, sampl_conf);
			phase_time_machine(&df_data_mapped, dfe_get_sampling_slot_samples_num(sampl_conf));

			results.phase = antenna_data_to_phase_diff(
				&df_data_mapped, DFE_ANT1, DFE_ANT2);
			results.azimuth = phase_to_angle(
				results.phase,
				DFE_ANT_D,
				(float)df_data_mapped.header.frequency * 1.0e6f);

			err = protocol_handling(sampl_conf, &df_data_mapped, &results);
			if (err) {
				printk("Error in protocol handling!\r\n");
				printk("Locator stopped!\r\n");
				return;
			}
		}
		else
		{
			printk("\r\nNo data received.");
		}
	 	k_sleep(K_MSEC(CONFIG_AOA_LOCATOR_DATA_SEND_WAIT_MS));
	}
}
