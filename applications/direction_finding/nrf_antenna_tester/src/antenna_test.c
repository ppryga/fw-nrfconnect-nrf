/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <string.h>
#include <assert.h>
#include <kernel.h>
#include <stdio.h>

#include "antenna_test.h"
#include "protocol.h"
#include "ble.h"
#include "dfe_local_config.h"
#include "dfe_samples_data.h"
#include "iq_samples_statistics.h"

/** @brief The highest antenna number that may be set for test */
#define ANT_TEST_MAX_ANT_NUMBER 12
/** @brief Value set in I or Q if over-saturation detected by radio */
#define IQ_OVERSATURATION_VAL (-32768)
/** @brief Frequency of a constant tone */
#define CTE_FREQUENCY_HZ 250000

#define MODULE ant_test
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DF_APP_LOG_LEVEL);

/** @brief Queue defined by BLE Controller to provide IQ samples data
 */
extern struct k_msgq df_packet_msgq;

/** @brief Initializes radio to run DFE
 *
 * @param[in]	sampl_conf pointer to sampling configuration
 * @param[in]	ant_conf pointer to antennas configuration
 *
 * @return 0 DFE was initialized successfully, non-zero value otherwise
 */
static int initlialize_dfe(const struct dfe_sampling_config* sampl_conf,
			   const struct dfe_antenna_config* ant_conf);

/** @brief Send IQ data
 *
 * @param[in] mapped_data	Pointer to IQ samples mapped to antennas
 * @param[in] sampl_conf	Pointer to sampling configuration
 *
 * return 0 if samples were sent successfully, other value means error
 */
static int send_iq_data(const struct dfe_mapped_packet *mapped_data,
			const struct dfe_sampling_config* sampl_conf);

/** @brief Collects IQ samples from Bluetooth controller
 *
 * Collects IQ samples and maps them to antennas numbers.
 *
 * @param[out] mapped_data	Pointer where to store IQ samples
 * 				mapped to antenna number
 * @param[in] ampl_conf		Pointer to sampling configuration
 * @param[in] ant_conf		Pointer to antenna configuration
 *
 * @retval 0	If samples were collected successfully
 * @retval -1	Timeout, no data were collected
 */
static int collect_iq_samples(struct dfe_mapped_packet *mapped_data,
			      const struct dfe_sampling_config* sampl_conf,
			      const struct dfe_antenna_config* ant_conf);

/** @brief Checks if any of IQ samples pairs is zero.
 *
 * Any samples (I,Q) should not hold zeroes.
 * If that happened it would mean there is no signal
 * because magnitude is zero.
 *
 * @param[in] mapped_data	Pointer to IQ samples mapped to antennas
 * @param[in] verbosity		Level of test log verbosity
 *
 * @return true if test finished successfully, false otherwise
 */
static bool test_are_iq_data_not_zeros(const struct dfe_mapped_packet *mapped_data,
				      enum test_verbosity_level verbosity)
{
	LOG_DBG("Test IQ data are not zeros started");

	int zeros_count = 0;
	int samples_tested = mapped_data->ref_data.samples_num;

	if (verbosity >= TEST_VERBOSITY_MED) {
		protocol_send_msg("[TEST] Check if IQ data are zeros:\r\n");
	}
	/* Check reference period */
	const struct dfe_ref_samples *ref_data = &mapped_data->ref_data;
	for(int idx = 0; idx < ref_data->samples_num; ++idx) {
		if (ref_data->data[idx].i == 0 &&
			ref_data->data[idx].q == 0) {
			++zeros_count;
			if(verbosity >= TEST_VERBOSITY_HIGH) {
				protocol_send_msg("\tSample: %d : I=%.3f, Q=%.3f ", idx,
						  ref_data->data[idx].i, ref_data->data[idx].q);
			}
		}
	}

	size_t index;
	/* Check regular samples */
	const struct dfe_samples *samples_data = &mapped_data->sampl_data[0];
	for(int idx = 0; idx < mapped_data->header.length; ++idx) {
		index = idx * samples_data->samples_num;
		const union dfe_iq_f *iq = &samples_data[idx].data[0];
		for(int iq_idx = 0; iq_idx < samples_data->samples_num; ++iq_idx) {
			if (iq[iq_idx].i == 0.0 && iq[iq_idx].q == 0.0) {
				++zeros_count;
				if(verbosity >= TEST_VERBOSITY_HIGH) {
					protocol_send_msg("\tSample: %d : I=%.3f, Q=%.3f ", index + iq_idx,
							 ref_data->data[idx].i, ref_data->data[idx].q);
				}
			}
		}
		samples_tested += samples_data->samples_num;
	}

	/* send summary */
	if (verbosity >= TEST_VERBOSITY_MED) {
		protocol_send_msg("\tNumber of samples tested: %d\r\n", samples_tested);
		protocol_send_msg("\tNumber of samples with zeros: %d\r\n", zeros_count);
	}
	if (zeros_count) {
		protocol_send_msg("\tTest FAILED\r\n");
	} else {
		protocol_send_msg("\tTest SUCCESS\r\n");
	}

	LOG_DBG("Test IQ data are not zeros finished");
	return (zeros_count == 0); //!< no zeros found mean success
}

/** @brief Checks if any of IQ samples holds over saturation information.
 *
 * Any samples pair should not hold value -32768 in I and Q data.
 * That means over saturation. If single value of pair I and Q has such value
 * there is something very wrong.
 *
 * @param[in] mapped_data	Pointer to IQ samples mapped to antennas
 * @param[in] verbosity		Level of test log verbosity
 *
 * @return 0 if test finished successfully, not zero otherwise
 */
static int test_are_iq_data_oversaturated(const struct dfe_mapped_packet *mapped_data,
					  enum test_verbosity_level verbosity)
{
	LOG_DBG("Test IQ data are not zeros started");

	int over_sat_count = 0;
	int samples_tested = mapped_data->ref_data.samples_num;

	protocol_send_msg("[TEST] Check if IQ data are over saturated:\r\n");

	/* Check reference period */
	const struct dfe_ref_samples *ref_data = &mapped_data->ref_data;
	for(int idx = 0; idx < ref_data->samples_num; ++idx) {
		if (ref_data->data[idx].i == IQ_OVERSATURATION_VAL &&
			ref_data->data[idx].q == IQ_OVERSATURATION_VAL) {
			++over_sat_count;
			if (verbosity >= TEST_VERBOSITY_HIGH) {
					protocol_send_msg("\tOversaturation in sample: %d \r\n", idx);
				}
		} else if ((ref_data->data[idx].i == IQ_OVERSATURATION_VAL ||
			    ref_data->data[idx].q == IQ_OVERSATURATION_VAL)) {
			if (verbosity >= TEST_VERBOSITY_HIGH) {
				protocol_send_msg("\tCorrupted sample: %d {I,Q}: {%f,  %f}\r\n",
						  idx, ref_data->data[idx].i, ref_data->data[idx].q);
			}
		}
	}

	/* Check regular samples */
	const struct dfe_samples *samples_data = &mapped_data->sampl_data[0];
	for(int idx = 0; idx < mapped_data->header.length; ++idx) {
		const union dfe_iq_f *iq = &samples_data[idx].data[0];
		for(int iq_idx = 0; iq_idx < samples_data->samples_num; ++iq_idx) {
			if (iq[idx].i == IQ_OVERSATURATION_VAL &&
				iq[idx].q == IQ_OVERSATURATION_VAL) {
				++over_sat_count;
				if (verbosity >= TEST_VERBOSITY_HIGH) {
					protocol_send_msg("\tOversaturation in sample: %d \r\n",
							  (idx * samples_data->samples_num + idx));
				}
			} else if ((iq[idx].i == IQ_OVERSATURATION_VAL ||
					   iq[idx].q == IQ_OVERSATURATION_VAL)) {
				if (verbosity >= TEST_VERBOSITY_HIGH) {
					protocol_send_msg("\tCorrupted sample: %d {I,Q}: {%f,  %f}\r\n",
							  (idx * samples_data->samples_num + idx),
							  iq[idx].i, iq[idx].q);
				}
			}
		}
		samples_tested += samples_data->samples_num;
	}

	/* send summary */
	if (verbosity >= TEST_VERBOSITY_MED) {
		protocol_send_msg("\tNumber of samples tested: %d\r\n", samples_tested);
		protocol_send_msg("\tNumber of over saturated samples: %d\r\n", over_sat_count);
	}
	if (over_sat_count) {
		protocol_send_msg("\tTest FAILED\r\n");
	} else {
		protocol_send_msg("\tTest SUCCESS\r\n");
	}

	LOG_DBG("Test IQ data are not zeros finished");
	return (over_sat_count == 0); //!< No over saturated samples found means success
}


/** @brief Check if IQ samples collected after reference are out of error
 * range.
 *
 * Evaluates phase value change of phase between consecutive samples.
 * The change is compared with expected phase, the difference should be in
 * provided range (-phase_range, phase_range).
 *
 * @param[in] mapped_data		Pointer to IQ samples mapped to antennas
 * @param[in] phase_range_deg	Range to be checked if phase is in between
 * @param[in] expected_phase_change	Value of expected phase change between
 * 					consecutive samples
 * @param[in] verbosity			Level of test log verbosity
 *
 * @return true if test finished successfully, false otherwise
 */
static bool test_if_phase_offset_in_error_range(const struct dfe_mapped_packet *mapped_data,
					       const struct dfe_sampling_config* sampl_conf,
					       float phase_range_deg,
					       float expected_phase_change,
					       enum test_verbosity_level verbosity)
{
	LOG_DBG("Test if phase offset between samples is in range");

	const union dfe_iq_f *iq1;
	float phase1;
	float phase2;
	float phase_diff;
	float diff_from_expected;
	size_t index;
	bool out_of_range;
	size_t samples_out_of_range = 0;
	size_t sampl_num;

	protocol_send_msg("[TEST]  Check if phase offset between samples is in range:\r\n");

	/* Check sample offset from average */
	const struct dfe_samples *samples_data = &mapped_data->sampl_data[0];
	for(int idx = 0; idx < mapped_data->header.length; ++idx) {
		iq1 = &samples_data[idx].data[0];
		index = idx * samples_data->samples_num;

		if (idx == mapped_data->header.length-1) {
			sampl_num = samples_data->samples_num -1;
		} else {
			sampl_num = samples_data->samples_num;
		}
		for(int iq_idx = 0; iq_idx < sampl_num; ++iq_idx) {
			out_of_range = false;
			phase1 = eval_sample_phase(iq1[iq_idx]);

			if(iq_idx >= samples_data->samples_num-1) {
				iq1 = &samples_data[idx+1].data[0];
				phase2 = eval_sample_phase(iq1[0]);
			} else {
				phase2 = eval_sample_phase(iq1[iq_idx+1]);
			}

			phase_diff = phase2 - phase1;
			phase_diff = wrapp_phase_around_pi(phase_diff);
			diff_from_expected = fabs(phase_diff - expected_phase_change);

			phase_diff = rad_to_deg(phase_diff);
			diff_from_expected = rad_to_deg(diff_from_expected);

			if( diff_from_expected > phase_range_deg) {
				out_of_range = true;
				samples_out_of_range++;
			}

			if(verbosity >= TEST_VERBOSITY_HIGH) {
				protocol_send_msg("\tSample: %d phase1: %.3f, phase2: %.3f , abs. diff. %.3f, diff. from expected: %.3f. allowed abs. diff.: %.3f ",
						  (index + iq_idx), rad_to_deg(phase1), rad_to_deg(phase2), phase_diff, diff_from_expected, phase_range_deg);
				if (out_of_range) {
					protocol_send_msg("Sample FAILED. \r\n");
				} else {
					protocol_send_msg("Sample CORRECT. \r\n");
				}
			}
		}
	}

	if (verbosity >= TEST_VERBOSITY_MED) {
		protocol_send_msg("\tTested samples: %d\r\n", (mapped_data->header.length * mapped_data->sampl_data[0].samples_num) - 1);
		protocol_send_msg("\tTested failed: %d\r\n", samples_out_of_range);
	}
	if (samples_out_of_range != 0) {
			protocol_send_msg("\tTest FAILED\r\n");
	} else {
		protocol_send_msg("\tTest SUCCESS\r\n");
	}

	return (samples_out_of_range == 0);
}

/** @brief Evaluates magnitude of IQ samples collected after
 * reference period.
 *
 * @param[in] mapped_data	Pointer to IQ samples mapped to antennas
 * @param[in] verbosity		Level of test log verbosity
 *
 */
static void stat_evaluate_magnitude_of_samples(const struct dfe_mapped_packet *mapped_data,
					     enum test_verbosity_level verbosity)
{
	LOG_DBG("Test if phase offset between samples is in range");

	const union dfe_iq_f *iq1;
	float magnitude;
	size_t index;

	if(verbosity >= TEST_VERBOSITY_HIGH) {
		protocol_send_msg("[STAT]  Evaluate samples magnitude:\r\n");
	}

	/* Check sample offset from average */
	const struct dfe_samples *samples_data = &mapped_data->sampl_data[0];
	for(int idx = 0; idx < mapped_data->header.length; ++idx) {
		iq1 = &samples_data[idx].data[0];
		index = idx * samples_data->samples_num;

		for(int iq_idx = 0; iq_idx < samples_data->samples_num; ++iq_idx) {
			magnitude = eval_sample_magnitude(iq1[iq_idx]);
			if(verbosity >= TEST_VERBOSITY_HIGH) {
				protocol_send_msg("\tSample: %d magnitude: %.3f \r\n",
						  (index + iq_idx), magnitude);
			}
		}
	}
}

/** @brief Antenna test procedure
 *
 * Test procedure is run for single antenna.
 * It is run number of times provided by @p number_of_cte_to_analyze.
 *
 * @param[in] antenna_num		Number of antenna to test
 * @param[in] number_of_cte_to_analyze	Number of CTE to be collected and tested
 * @param[in] verbosity			Level of test log verbosity
 *
 * @return true if test finished successfully, false otherwise
 */
static bool test_antenna(u8_t antenna_num, u8_t number_of_cte_to_analyze,
			enum test_verbosity_level verbosity)
{
	assert(antenna_num != 0);
	assert(antenna_num <= ANT_TEST_MAX_ANT_NUMBER);

	int err;
	bool test_result = true;
	const struct dfe_sampling_config* sampl_conf = NULL;
	const struct dfe_antenna_config* ant_conf = NULL;

	sampl_conf = dfe_get_sampling_config();
	ant_conf = dfe_get_antenna_config();

	dfe_set_single_antenna_for_whole_cte(antenna_num);
	err = initlialize_dfe(sampl_conf, ant_conf);
	if (err) {
		LOG_ERR("Error while DFE initialization: %d", err);
		return false;
	}
	err = bt_start_scanning();
	if (err) {
		return false;
	}

	static struct dfe_mapped_packet df_mapped_data;
	u16_t samples_spacing_ns = dfe_get_sample_spacing_ref_ns(sampl_conf->sample_spacing_ref);

	float avg_phase_off_sampl;
	float avg_phase_off_periods;
	float avg_magnitude;
	float expected_phase_diff;

	expected_phase_diff= get_expected_phase_offset(CTE_FREQUENCY_HZ, samples_spacing_ns);

	memset(&df_mapped_data, 0, sizeof(df_mapped_data));

	for( u8_t idx = 0; idx < number_of_cte_to_analyze; ++idx) {
		err = collect_iq_samples(&df_mapped_data, sampl_conf, ant_conf);
		if (err) {
			LOG_ERR("Error while collecting IQ data, CTE numb: %d", idx);
			return false;
		}

		evaluate_stats_in_ref(&df_mapped_data.ref_data, sampl_conf,
				      CTE_FREQUENCY_HZ,
				      &avg_phase_off_sampl, &avg_phase_off_periods,
				      &avg_magnitude);
		protocol_send_msg("[STAT] Reference statistics:\r\n");
		if (verbosity >= TEST_VERBOSITY_MED) {
			protocol_send_msg("\tAvg. phase offset between samples [deg]: %.3f \r\n", avg_phase_off_sampl);
			protocol_send_msg("\tAvg. phase offset between periods [deg]: %.3f \r\n", avg_phase_off_periods);
			protocol_send_msg("\tAvg. magnitude: %.3f \r\n", avg_magnitude);
		}
		if (!test_are_iq_data_not_zeros(&df_mapped_data, verbosity)) {
			test_result = false;
		}
		if (!test_are_iq_data_oversaturated(&df_mapped_data, verbosity)) {
			test_result = false;
		}
		if (!test_if_phase_offset_in_error_range(&df_mapped_data, sampl_conf,
						(float)CONFIG_ANT_TEST_PHASE_OFFSET_DEVIATION_RANGE_DEG,
						    expected_phase_diff, verbosity)) {
			test_result = false;
		}
		stat_evaluate_magnitude_of_samples(&df_mapped_data, verbosity);

		if (verbosity >= TEST_VERBOSITY_HIGH) {
			protocol_send_msg("[STAT] Raw data:\r\n");
			err = send_iq_data(&df_mapped_data, sampl_conf);
			if (err) {
				LOG_ERR("Error while collecting IQ data, CTE numb: %d", idx);
				return false;
			}
		}
	}

	err = bt_stop_scanning();
	if (err) {
		return false;
	}

	return test_result;
}

bool run_antenna_test_suite(enum test_verbosity_level verbosity)
{
	bool result;
	bool global_result = true;
	bool ant_test_results[ANT_TEST_MAX_ANT_NUMBER];

	protocol_send_msg("[TEST] Start antennas matrix test\r\n");

	for (int ant_idx = 1; ant_idx <= ANT_TEST_MAX_ANT_NUMBER; ++ant_idx) {
		protocol_send_msg("[TEST] Antenna %d test START.\r\n", ant_idx);
		result = test_antenna(ant_idx, CONFIG_ANT_TEST_NUMBER_OF_CTE_TO_COLLECT, verbosity);
		if (!result) {
			protocol_send_msg("[TEST] Antenna %d test FILED. Error: %d\r\n", ant_idx, result);
		} else {
			protocol_send_msg("[TEST] Antenna %d test SUCCSESS\r\n", ant_idx);
		}
		ant_test_results[ant_idx-1] = result;
	}

	protocol_send_msg("[SUMMARY]\r\n");
	for (int ant_idx = 0; ant_idx < ANT_TEST_MAX_ANT_NUMBER; ++ant_idx) {
		if (!ant_test_results[ant_idx]) {
			protocol_send_msg("[TEST] Antenna %d test FILED.\r\n", ant_idx+1);
			global_result = false;
		} else {
			protocol_send_msg("[TEST] Antenna %d test SUCCSESS\r\n", ant_idx+1);
		}
	}
	protocol_send_msg("[TEST] End antennas matrix test\r\n");

	return global_result;
}

/****************************************************************************
 * Helper functions for setup and reporting
 ****************************************************************************/
static int initlialize_dfe(const struct dfe_sampling_config* sampl_conf,
			   const struct dfe_antenna_config* ant_conf)
{
	const struct dfe_ant_gpio* ant_gpio = NULL;
	u8_t ant_gpio_len = 0;

	ant_gpio_len = dfe_get_ant_gpios_config_len();
	ant_gpio = dfe_get_ant_gpios_config();

	LOG_DBG("Initialize DFE");
	int err = dfe_init(sampl_conf, ant_conf, ant_gpio, ant_gpio_len);

	if (err) {
		LOG_ERR("Locator stopped!");
		return err;
	}

	return 0;
}

static int collect_iq_samples(struct dfe_mapped_packet *mapped_data,
			      const struct dfe_sampling_config* sampl_conf,
			      const struct dfe_antenna_config* ant_conf)
{
	assert( mapped_data != NULL);

	struct dfe_packet df_data_packet;
	memset(&df_data_packet, 0, sizeof(df_data_packet));

	k_msgq_get(&df_packet_msgq, &df_data_packet,
		   K_MSEC(CONFIG_ANT_TEST_WAIT_TIME_FOR_CTE_MS));
	if (df_data_packet.hdr.length == 0) {
		LOG_DBG("No data received.");
		return -1;
	}

	LOG_DBG("Data arrived...");

	dfe_map_iq_samples_to_antennas(mapped_data, &df_data_packet, sampl_conf,
				       ant_conf);
	return  0;
}

static int send_iq_data(const struct dfe_mapped_packet *mapped_data,
			const struct dfe_sampling_config* sampl_conf)
{
	int err = protocol_handling(sampl_conf, mapped_data);

	if (err) {
		LOG_ERR("Error in protocol handling!");
		LOG_ERR("Locator stopped!");
		protocol_send_msg("[TEST] Test failed\r\n");
		return err;
	}
	return 0;
}

