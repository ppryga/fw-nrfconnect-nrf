/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <ztest.h>
#include "configuration_fixtures.h"
#include "dfe_configuration_tests.h"
#include "configuration_fixtures.h"
#include <dfe_local_config.h>

void test_sampling_type_when_oversampling_configured()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampl_config);
	zassert_equal(DFE_OVER_SAMPLING, sampling_type, "Wrong sampling type");
}

void test_sampling_type_when_bt_1us_slot_configured()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampl_config);
	zassert_equal(DFE_REGULAR_SAMPLING, sampling_type, "Wrong sampling type");
}

void test_sampling_type_when__bt_2us_slot_configured()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampl_config);
	zassert_equal(DFE_REGULAR_SAMPLING, sampling_type, "Wrong sampling type");
}

void test_sampling_type_when_undersampling_configured_ommit_2nd_slot()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampl_config);
	zassert_equal(DFE_UNDER_SAMPLING, sampling_type, "Wrong sampling type");
}

void test_sampling_type_when_undersampling_configured_ommit_4nd_slot()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampl_config);
	zassert_equal(DFE_UNDER_SAMPLING, sampling_type, "Wrong sampling type");
}

void test_calculation_of_effective_slots_number_for_oversampling()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	const uint16_t expected_slots_num = 148; // (148 / 2us) * 2 -> CTE divided by ant switch, times 2 because of oversampling
	zassert_equal(effective_slots_num, expected_slots_num,
				  "Wrong slots number for oversampling configuration");
}

void test_calculation_of_effective_slots_number_for_ble_1us_slot()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	const uint16_t expected_slots_num = 74; // 148us / 2us = 74 - sample is taken only in sampling slot
	zassert_equal(effective_slots_num, expected_slots_num,
				  "Wrong slots number for regular configuration");
}

void test_calculation_of_effective_slots_number_for_ble_2us_slot()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	const uint16_t expected_slots_num = 37; // 148us / 4us = 37 - sample is taken only in sampling slot
	zassert_equal(effective_slots_num, expected_slots_num,
				  "Wrong slots number for regular configuration");
}

void test_calculation_of_effective_slots_number_for_under_sampling()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	printk("ef slots num %d", effective_slots_num);
	const uint16_t expected_slots_num = 37; // 148us/2us/(2us/4us) = 37 - sample is taken every second sampling slot
	zassert_equal(effective_slots_num, expected_slots_num,
				  "Wrong slots number for under sampling configuration");
}
