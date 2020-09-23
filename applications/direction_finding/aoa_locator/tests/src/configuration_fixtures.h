/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef TESTS_SRC_CONFIGURATION_FIXTURES_H_
#define TESTS_SRC_CONFIGURATION_FIXTURES_H_

enum sample_spacing {
	TSAMPLESPACING_4us = (1UL), 	/*!< 1us */
	TSAMPLESPACING_2us = (2UL), 	/*!< 2us */
	TSAMPLESPACING_1us = (3UL), 	/*!< 1us */
	TSAMPLESPACING_500ns = (4UL), 	/*!< 0.5us */
	TSAMPLESPACING_250ns = (5UL), 	/*!< 0.25us */
	TSAMPLESPACING_125ns = (6UL), 	/*!< 0.125us */
};

enum ref_spacing {
	TSAMPLESPACING_REF_4us = (1UL), 	/*!< 1us */
	TSAMPLESPACING_REF_2us = (2UL), 	/*!< 2us */
	TSAMPLESPACING_REF_1us = (3UL), 	/*!< 1us */
	TSAMPLESPACING_REF_500ns = (4UL), 	/*!< 0.5us */
	TSAMPLESPACING_REF_250ns = (5UL), 	/*!< 0.25us */
	TSAMPLESPACING_REF_125ns = (6UL), 	/*!< 0.125us */
};

enum switch_spacing {
	TSWITCHSPACING_4us = (1UL), 	/*!< 1us */
	TSWITCHSPACING_2us = (2UL), 	/*!< 2us */
	TSWITCHSPACING_1us = (3UL), 	/*!< 1us */
};

/** @brief Fixture to prepare IQ sampling configuration with settings
 * that enable oversampling.
 *
 * The function prepares configuration of CTE receive with following
 * parameters:
 * - CTE duration
 * - antenna switch spacing
 * - samples spacing in reference period
 * - samples spacing in antenna switching period
 * The configuration here is an oversampling it means there will be more
 * samples than allowed by BLE Specification.
 *
 * @Note Pay attention that Nordics radio does sampling also in switch slots.
*/
void setup_fixture_prepare_over_sampling();

/** @brief Fixture to prepare IQ sampling configuration with settings
 * that enable regular sampling with 1us switch slot (BLE compliant configuration).
 *
 * The function prepares configuration of CTE receive with following
 * parameters:
 * - CTE duration
 * - antenna switch spacing
 * - samples spacing in reference period
 * - samples spacing in antenna switching period
 * The configuration here is a regular sampling it complies with BLE Specification.
 * Also it means there will be the same number of samples as number of sampling slots.
*/
void setup_fixture_prepare_bt_sampling_1us_switch_slot();

/** @brief Fixture to prepare IQ sampling configuration with settings
 * that enable regular sampling with 2us switch slot (BLE compliant configuration).
 *
 * The function prepares configuration of CTE receive with following
 * parameters:
 * - CTE duration
 * - antenna switch spacing
 * - samples spacing in reference period
 * - samples spacing in antenna switching period
 * The configuration here is a regular sampling it complies with BLE Specification.
 * Also it means there will be the same number of samples as number of sampling slots.
*/
void setup_fixture_prepare_bt_sampling_2us_switch_slot();

/** @brief Fixture to prepare IQ sampling configuration with settings
 * that enable under sampling with every 2nd sampling slot taken.
 *
 * The function prepares configuration of CTE receive with following
 * parameters:
 * - CTE duration
 * - antenna switch spacing
 * - samples spacing in reference period
 * - samples spacing in antenna switching period
 * The configuration here is an under sampling it does not comply with BLE Specification.
 * Also it means there will be a sample provided every n-th sampling slot.
*/
void setup_fixture_prepare_undersampling_sample_every_2nd_slot();

/** @brief Fixture to prepare IQ sampling configuration with settings
 * that enable under sampling with every 4th sampling slot taken.
 *
 * The function prepares configuration of CTE receive with following
 * parameters:
 * - CTE duration
 * - antenna switch spacing
 * - samples spacing in reference period
 * - samples spacing in antenna switching period
 * The configuration here is an under sampling it does not comply with BLE Specification.
 * Also it means there will be a sample provided every n-th sampling slot.
*/
void setup_fixture_prepare_undersampling_sample_every_4th_slot();

/** @brief Common teardown procedure
 */
void common_teardown();

/** @brief Returns test instance of sampling configuration
 *
 * @return dfe_sampling_config* Pointer to sampling configuration instance.
 */
struct dfe_sampling_config *get_test_sampl_config();

/** @brief Returns test instance of antennas configuration
 *
 * @return dfe_antenna_config* Pointer to antenna configuration instance.
 */
struct dfe_antenna_config *get_test_ant_config();

#endif /* TESTS_SRC_CONFIGURATION_FIXTURES_H_ */
