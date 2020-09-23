/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>

#include <dfe_local_config.h>
#include "configuration_fixtures.h"

/** @brief Test instance for the DFE configuration */
static struct dfe_sampling_config g_test_sampl_config = {
	.dfe_mode = RADIO_DFEMODE_DFEOPMODE_AoA,
	.start_of_sampl = RADIO_DFECTRL1_DFEINEXTENSION_CRC,
	.number_of_8us = CONFIG_BT_CTLR_DFE_NUMBER_OF_8US,
	.en_sampling_on_crc_error = false,
	.dfe_trigger_task_only = true,
	.sampling_type = RADIO_DFECTRL1_SAMPLETYPE_IQ,
	.sample_spacing_ref = CONFIG_BT_CTLR_DFE_SAMPLE_SPACING_REF_VAL,
	.sample_spacing = CONFIG_BT_CTLR_DFE_SAMPLE_SPACING_VAL,
	.sample_offset = 1,
	.switch_spacing = CONFIG_BT_CTLR_DFE_SWITCH_SPACING_VAL,
	.switch_offset = 0,
	.guard_period_us = 4,
	.ref_period_us = 8,
};

/** @brief Test instance for the DFE antenna configuration */
static struct dfe_antenna_config g_test_ant_conf = {
	.ref_ant_idx = 11,
	.idle_ant_idx = 11,
	.ant_gpio_pattern = {0, 5, 6, 4, 9, 10, 8, 13, 14, 12, 1, 2, 0},
	.ant_gpio_pattern_len = 13,
	.antennae_switch_idx = {12,1,2,10,3,9,4,8,7,6,5},
	.antennae_switch_idx_len = 11
};

void setup_fixture_prepare_over_sampling()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_250ns;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_250ns;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_2us;
}

void prepare_config_cte160_sampling2us_switch2us()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_1us;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_2us;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_2us;
}

void setup_fixture_prepare_bt_sampling_1us_switch_slot()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_1us;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_2us;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_2us;
}

void setup_fixture_prepare_bt_sampling_2us_switch_slot()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_1us;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_4us;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_4us;
}

void setup_fixture_prepare_undersampling_sample_every_2nd_slot()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_1us;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_4us;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_2us;
}

void setup_fixture_prepare_undersampling_sample_every_4th_slot()
{
	g_test_sampl_config.number_of_8us = 20;	/* 20 * 8us = 160us */
	g_test_sampl_config.sample_spacing_ref = TSAMPLESPACING_REF_1us;
	g_test_sampl_config.sample_spacing = TSAMPLESPACING_4us;
	g_test_sampl_config.switch_spacing = TSWITCHSPACING_1us;
}

void common_teardown()
{

}

struct dfe_sampling_config  *get_test_sampl_config()
{
	return &g_test_sampl_config;
}

struct dfe_antenna_config *get_test_ant_config()
{
	return &g_test_ant_conf;
}
