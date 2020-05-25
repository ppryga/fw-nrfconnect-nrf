/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <zephyr/types.h>
#include "dfe_local_config.h"
#include "iq_samples_statistics.h"

/** @brief Number of wave periods in reference data */
#define PERIODS_NUM_IN_REF 2

#define MODULE ant_iq_stats
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DF_APP_LOG_LEVEL);

/** @brief Evaluate average phase change in reference period based on difference
 * of phase in two consecutive wave periods
 *
 * The function evaluates average phase offset(or phase drift) in reference
 * period. The offset returned is an angle value in degrees. Value is an average
 * change of phase between samples (or in a samples spacing time).
 *
 * @note There is an assumption that reference data have samples from at least
 * two complete wave periods. That assumption is required by algorithm used
 * to evaluate phase offset. It is done by subtraction of phase of corresponding
 * samples in consecutive periods.
 *
 * @param[in] dfe_ref_samples	IQ samples from reference period
 * @param[in] samples_in_period	Number of samples in single wave period
 *
 * @return Average phase offset in degrees
 */
static float eval_avg_phase_offset_in_ref_period_comp(const struct dfe_ref_samples *ref_data,
						      int samples_in_period)
{
	assert((ref_data->samples_num % samples_in_period) == 0);

	float phase1, phase2, diff;
	float temp_sum = 0.0;

	for (int idx = 0; idx < samples_in_period; ++idx) {
		phase1 = eval_sample_phase(ref_data->data[idx]);
		phase2 = eval_sample_phase(ref_data->data[idx + samples_in_period]);

		diff = phase1 - phase2;
		diff = wrapp_phase_around_pi(diff);

		temp_sum += diff;
	}

	float avg_phase_diff = temp_sum/(float)samples_in_period;

	/* change RAD to DEG */
	avg_phase_diff = rad_to_deg(avg_phase_diff);
	return avg_phase_diff;
}

/** @brief Evaluate average phase change in reference period based on phase
 * differences between consecutive samples.
 *
 * The function evaluates average phase offset error. The error is a difference
 * between actual phase offset of consecutive samples and ideal phase offset
 * evaluated for a given frequency. *
 *
 * @param[in] dfe_ref_samples		IQ samples from reference period
 * @param[in] frequency			Expected ideal frequency in Hz
 * @param[in] samples_spacign_ns	Time delay between consecutive samples in nanoseconds
 *
 * @return Average phase offset in degrees
 */
static float eval_avg_phase_offset_in_ref_samples_diff(const struct dfe_ref_samples *ref_data,
						       float frequency, u16_t samples_spacing_ns)
{
	assert(ref_data != NULL);
	assert(frequency != 0);
	assert(samples_spacing_ns != 0);

	float phase1, phase2, diff;
	float temp_sum = 0.0;
	float expected_phase_diff;

	expected_phase_diff = get_expected_phase_offset(frequency, samples_spacing_ns);

	for (int idx = 0; idx < ref_data->samples_num-1; ++idx) {
		phase1 = eval_sample_phase(ref_data->data[idx]);
		phase2 = eval_sample_phase(ref_data->data[idx+1]);

		diff = phase2 - phase1;
		diff = wrapp_phase_around_pi(diff);

		if (diff < expected_phase_diff)
			diff = expected_phase_diff - diff;
		else
			diff = diff - expected_phase_diff;

		temp_sum += diff;
		LOG_DBG("Ref sample phase diff: %.3f\r\n", rad_to_deg(diff));
	}

	float avg_phase_diff = temp_sum/(float)ref_data->samples_num;

	/* change RAD to DEG */
	avg_phase_diff = avg_phase_diff * 180.0f/M_PI;
	return avg_phase_diff;
}

/** @brief Evaluate average magnitude in reference period
 *
 * @param[in] dfe_ref_samples	IQ samples from reference period
 *
 * @return Evaluated average magnitude value
 */
static float eval_avg_magnitude_in_ref(const struct dfe_ref_samples *ref_data)
{
	assert(ref_data != NULL);

	float temp_mag_sum = 0.0;

	for (int idx = 0; idx < ref_data->samples_num; ++idx) {
		temp_mag_sum += eval_sample_magnitude(ref_data->data[idx]);
	}

	return temp_mag_sum/ref_data->samples_num;
}

void evaluate_stats_in_ref(const struct dfe_ref_samples* ref_samples,
			   const struct dfe_sampling_config* sampl_conf,
			   u32_t frequency,
			   float *avg_phase_off_sampl,
			   float *avg_phase_off_periods,
			   float *avg_mag)
{
	assert(ref_samples != NULL);
	assert(sampl_conf != NULL);
	assert(avg_phase_off_sampl != NULL);
	assert(avg_phase_off_periods != NULL);
	assert(avg_mag != NULL);

	float avg_value;
	u16_t samples_spacing_ns = dfe_get_sample_spacing_ref_ns(sampl_conf->sample_spacing_ref);
	u8_t samples_in_period = dfe_get_ref_samples_num(sampl_conf) / PERIODS_NUM_IN_REF;

	avg_value = eval_avg_phase_offset_in_ref_samples_diff(ref_samples, frequency, samples_spacing_ns);
	*avg_phase_off_sampl = avg_value;
	avg_value = eval_avg_phase_offset_in_ref_period_comp(ref_samples, samples_in_period);
	*avg_phase_off_periods = avg_value;
	*avg_mag = eval_avg_magnitude_in_ref(ref_samples);
}
