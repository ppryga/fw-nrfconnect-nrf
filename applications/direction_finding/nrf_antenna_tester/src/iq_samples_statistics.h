/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef DF_IQ_SAMPLES_STATISTICS_H_
#define DF_IQ_SAMPLES_STATISTICS_H_

#include <complex.h>
#include <math.h>
#include "dfe_samples_data.h"

#ifndef M_PI
/** PI constant */
#define M_PI		3.14159265358979323846
#endif

/** @brief An alias for 360 degree */
#define FULL_ANGLE 360.0
/** @brief Multiplier to change nanosecond to seconds */
#define NS_TO_SEC_MULT (1.0e-9)

/** @brief Evaluates phase of a IQ sample
 *
 * @param[in] iq_sample IQ representation of a sample
 *
 * @return Value of a phase
 */
static inline float eval_sample_phase(union dfe_iq_f iq_sample)
{
	float complex complex_sample;

	complex_sample = iq_sample.i + iq_sample.q*I;
	return cargf(complex_sample);
}

/** @brief Evaluates magnitude of a IQ sample
 *
 * @param[in] iq_sample IQ representation of a sample
 *
 * @return Value of a magnitude
 */
static inline float eval_sample_magnitude(union dfe_iq_f iq_sample)
{
	return cabs(iq_sample.i + iq_sample.q*I);
}

/** @brief Wrap phase angles if exceed PI or -PI
 *
 * @param[in] phase	Value in radians to be wrapped
 *
 * @return Wrapped phase value in RAD
 */
static inline float wrapp_phase_around_pi(float phase)
{
	if( phase > M_PI ) {
		do {
			phase -= 2.0f * M_PI;
		} while( phase > M_PI );
	}
	if (phase < -M_PI ) {
		do {
			phase += 2.0f * M_PI;
		} while (phase < -M_PI );
	}

	return phase;
}

/** @brief Wrap phase angles if exceed 180 or -180 degrees
 *
 * @param[in] phase	Value in degrees to be wrapped
 *
 * @return Wrapped phase value in degrees
 */
static inline float wrapp_phase_around_180(float phase)
{
	if( phase > 180.0 ) {
		do {
			phase -= 360;
		} while( phase > 180.0 );
	}
	if (phase < -180 ) {
		do {
			phase += 360;
		} while( phase < -180.0 );
	}

	return phase;
}

/** @brief Converts angle values from radians to degrees
 *
 * @param[in] rad	Angle value in radians
 *
 * @return Angle value in degrees
 */
static inline float rad_to_deg(float rad) {
	return rad * 180 / M_PI;
}

/** @brief Converts angle values from degrees to radians
 *
 * @param[in] rad	Angle value in degrees
 *
 * @return Angle value in radians
 */
static inline float deg_to_rad(float rad) {
	return rad * M_PI / 180.0;
}

/** @brief Evaluates expected phase diff for a weve with given frequency and
 * samples spacing
 *
 * @param[in] frequency				Frequency in Hz
 * @param[in] samples_spacing_ns	Spacing between consecutive samples in nanoseconds
 *
 * @return Angle value in radians
 */
static inline float get_expected_phase_offset(float frequency, u16_t samples_spacing_ns)
{
	float expected_phase_diff;

	expected_phase_diff = FULL_ANGLE / ((1.0f/frequency) / (samples_spacing_ns * NS_TO_SEC_MULT));
	expected_phase_diff = deg_to_rad(expected_phase_diff);
	return expected_phase_diff;
}
/** @brief Evaluates statistics for IQ data in reference period
 *
 * Evaluates following statistic for reference IQ samples:
 * - average average phase offset per sample
 * - average magnitude
 *
 * @param[in] ref_samples				Pointer to reference period data
 * @param[in] sampl_conf 				Pointer to sampling configuration
 * @param[in] frequency					Expected ideal frequency in Hz
 * @param[out] avg_phase_off_sampl		Pointer to return evaluated average phase based
 * 										on phase difference of consecutive samples
 * @param[out] avg_phase_off_periods	Pointer to return evaluated average phase based
 * 										on phase difference of consecutive periods
 * @param[out] avg_mag					Pointer to return evaluated average magnitude
 */
void evaluate_stats_in_ref(const struct dfe_ref_samples* ref_samples,
						   const struct dfe_sampling_config* sampl_conf,
						   u32_t frequency,
						   float *avg_phase_ofF_sampl,
						   float *avg_phase_off_periods,
						   float *avg_mag);

#endif /* DF_IQ_SAMPLES_STATISTICS_H_ */
