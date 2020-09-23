/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <stdbool.h>
#include <assert.h>
#include <zephyr/types.h>
#include <kernel.h>
#include <nrf.h>

#include "dfe_local_config.h"

const static struct dfe_sampling_config g_sampl_config = {
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

const static struct dfe_antenna_config g_ant_conf = {
	.ref_ant_idx = 11,
	.idle_ant_idx = 11,
	.ant_gpio_pattern = {0, 5, 6, 4, 9, 10, 8, 13, 14, 12, 1, 2, 0},
	.ant_gpio_pattern_len = 13,
	.antennae_switch_idx = {12,1,2,10,3,9,4,8,7,6,5},
	.antennae_switch_idx_len = 11
};

const static struct dfe_ant_gpio g_gpio_conf[4] = {
		{0, 3}, {1,4}, {2, 28}, {3,29}
};

/** @brief Creates switching pattern array
 *
 * The function converts provided antenna indices into array of
 * switch patterns that will be applied by radios DFE module to switch antennas.
 *
 * @param[out]	array				Pointer where to store patterns
 * @param[in]	len					Length of @p array
 * @param[in]	gpio_patterns		Array of GPIO patterns
 * @param[in]	gpio_patterns_len	Length of @p gpio_patterns
 * @param[in]	switch_array		Array with antenna indices
 * @param[in]	switch_array_len	Length of @p switch_array
 *
 * @retval 0		switch pattern array created successfully
 * @retval -EINVAL	one of pointers is NULL or one of lengths is zero
 */
static int create_switching_pattern_array(u8_t *array, u8_t len,
		const u8_t *gpio_patterns, u8_t gpio_patterns_len,
		const u8_t *switch_array, u8_t switch_array_len);

/** @brief Evaluates duration of antenna switching period
 *
 * @param[in] sampling_conf	Sampling configuration
 *
 * @return Duration in [ns]
 */
static u32_t get_switching_duration_ns(const struct dfe_sampling_config *sampling_conf);


const struct dfe_sampling_config* dfe_get_sampling_config()
{
	return &g_sampl_config;
}

const struct dfe_antenna_config* dfe_get_antenna_config()
{
	return &g_ant_conf;
}

const struct dfe_ant_gpio* dfe_get_ant_gpios_config()
{
	return &g_gpio_conf[0];
}

u8_t dfe_get_ant_gpios_config_len()
{
	return ARRAY_SIZE(g_gpio_conf);
}

int dfe_init(const struct dfe_sampling_config *sampl_conf,
	     const struct dfe_antenna_config *ant_conf,
	     const struct dfe_ant_gpio *ant_gpio, u8_t ant_gpio_len)
{
	assert(sampl_conf != NULL);
	assert(ant_conf != NULL);
	assert(ant_gpio != NULL);
	assert(ant_gpio_len != 0);

	int err = 0;

	err = dfe_set_mode(sampl_conf->dfe_mode);
	if (err) {
		printk("[DFE] - DFE mode is unknown\n");
		return -EINVAL;
	}

	err = dfe_set_duration(sampl_conf->number_of_8us);
	if(err) {
		printk("Error! Number of 8us periods is out of allowed range!\r\n");
		return err;
	}

	err = dfe_set_start_point(sampl_conf->start_of_sampl);
	if(err) {
		printk("Error! DFE start point value is out of allowed range!\r\n");
		return err;
	}

	dfe_set_sample_on_crc_error(sampl_conf->en_sampling_on_crc_error);

	dfe_set_trig_dfe_start_task_only(true);

	err = dfe_set_sampling_spacing_ref(sampl_conf->sample_spacing_ref);
	if(err) {
		printk("Error! Reference sample spacing value is out of range!\r\n");
		return err;
	}
	err = dfe_set_sampling_type(sampl_conf->sampling_type);
	if(err) {
		printk("Error! Sampling type value is out of range!\r\n");
		return err;
	}
	err = dfe_set_sample_spacing(sampl_conf->sample_spacing);
	if(err) {
		printk("Error! Sample spacing value is out of range!\r\n");
		return err;
	}
	err = dfe_set_backoff_gain(0);
	if(err) {
		printk("Error! Gain backoff value is out of range!\r\n");
		return err;
	}
	err = dfe_set_switch_offset(sampl_conf->switch_offset);
	if(err) {
		printk("Error! Switch offset value is out of range!\r\n");
		return err;
	}
	err = dfe_set_sample_offset(sampl_conf->sample_offset);
	if(err) {
		printk("Error! Sampling offset value is out of range!\r\n");
		return err;
	}

	err = dfe_set_ant_switch_spacing(sampl_conf->switch_spacing);
	if(err) {
		printk("Error! Antenna switch spacing is out of range!\r\n");
		return err;
	}

	err = dfe_set_ant_gpios(ant_gpio, ant_gpio_len);
	if(err) {
		printk("Error! Number of GPIO for antennas switching is to large!\r\n");
		return err;
	}

	u8_t pattern_array[ant_conf->antennae_switch_idx_len];
	err = create_switching_pattern_array(pattern_array,
				       ant_conf->antennae_switch_idx_len,
				       ant_conf->ant_gpio_pattern,
				       ant_conf->ant_gpio_pattern_len,
				       ant_conf->antennae_switch_idx,
				       ant_conf->antennae_switch_idx_len);
	if(err) {
		printk("Error! Preparation of switch patterns array failed!\r\n");
		return err;
	}

	err = dfe_set_ant_gpio_patterns(ant_conf->ant_gpio_pattern[ant_conf->idle_ant_idx],
					ant_conf->ant_gpio_pattern[ant_conf->ref_ant_idx],
					pattern_array,
					ant_conf->antennae_switch_idx_len);
	if(err) {
		printk("Error! Initialization of gpio patterns failed!\r\n");
		return err;
	}
	return 0;
}

static int create_switching_pattern_array(u8_t *array, u8_t len,
					  const u8_t *gpio_patterns,
					  u8_t gpio_patterns_len,
					  const u8_t *switch_array,
					  u8_t switch_array_len)
{

	if (array == NULL || len == 0 || gpio_patterns == NULL ||
	    gpio_patterns_len == 0 || switch_array == NULL ||
	    switch_array_len == 0 || len < switch_array_len) {
		return -EINVAL;
	}

	for (u8_t idx = 0; idx < switch_array_len; ++idx) {
		if (switch_array[idx] >= gpio_patterns_len)
			return -ERANGE;
	}

	for (u8_t idx = 0; idx < len; ++idx) {
		array[idx] = gpio_patterns[switch_array[idx]];
	}

	return 0;
}

static u32_t get_switching_duration_ns(const struct dfe_sampling_config *sampling_conf)
{
	assert(sampling_conf != NULL);

	u32_t swiching_duration_us = ((sampling_conf->number_of_8us * DFE_US(8)) -
			(sampling_conf->guard_period_us + sampling_conf->ref_period_us));

	return (swiching_duration_us * DFE_NS(1000));
}

uint16_t dfe_get_effective_slots_num(const struct dfe_sampling_config *sampling_conf)
{
	assert(sampling_conf != NULL);

	u32_t switching_duration_ns = get_switching_duration_ns(sampling_conf);
	u32_t switch_spacing_ns = dfe_get_switch_spacing_ns(sampling_conf->switch_spacing);

	u16_t effective_slots_num = (switching_duration_ns / switch_spacing_ns);
	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampling_conf);

	switch(sampling_type)
	{
		case DFE_OVER_SAMPLING:
			effective_slots_num = (effective_slots_num*2);
			break;
		case DFE_UNDER_SAMPLING:
		{
			u8_t ant_num_divider = dfe_get_sample_spacing_ns(sampling_conf->sample_spacing) /
							       dfe_get_switch_spacing_ns(sampling_conf->switch_spacing);
			effective_slots_num = (effective_slots_num / ant_num_divider);
			break;
		}
		case DFE_REGULAR_SAMPLING:
		default:
			break;
	}

	return effective_slots_num;
}

u16_t dfe_get_switch_spacing_ns(u8_t spacing)
{
	u16_t spacing_ns = 0;
	switch(spacing) {
		case 0:
			spacing_ns = DFE_NS(8000);
			break;
		case RADIO_DFECTRL1_TSWITCHSPACING_4us:
			spacing_ns = DFE_NS(4000);
			break;
		case RADIO_DFECTRL1_TSWITCHSPACING_2us:
			spacing_ns = DFE_NS(2000);
			break;
		case RADIO_DFECTRL1_TSWITCHSPACING_1us:
			spacing_ns = DFE_NS(1000);
			break;
		default:
			return -1;
	}
	return spacing_ns;
}

u16_t dfe_get_sample_spacing_ns(u8_t sampling)
{
	u16_t sampling_ns = 0;
	switch(sampling) {
		case RADIO_DFECTRL1_TSAMPLESPACING_4us:
			sampling_ns = DFE_NS(4000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACING_2us:
			sampling_ns = DFE_NS(2000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACING_1us:
			sampling_ns = DFE_NS(1000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACING_500ns:
			sampling_ns = DFE_NS(500);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACING_250ns:
			sampling_ns = DFE_NS(250);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACING_125ns:
			sampling_ns = DFE_NS(125);
			break;
		default:
			sampling_ns = -1;
	}
	return sampling_ns;
}

u16_t dfe_get_sample_spacing_ref_ns(u8_t sampling) {

	u16_t sampling_ns = 0;
	switch(sampling) {
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_4us:
			sampling_ns = DFE_NS(4000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_2us:
			sampling_ns = DFE_NS(2000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_1us:
			sampling_ns = DFE_NS(1000);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_500ns:
			sampling_ns = DFE_NS(500);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_250ns:
			sampling_ns = DFE_NS(250);
			break;
		case RADIO_DFECTRL1_TSAMPLESPACINGREF_125ns:
			sampling_ns = DFE_NS(125);
			break;
		default:
			sampling_ns = -1;
	}
	return sampling_ns;
}

u16_t dfe_get_sampling_slot_samples_num(const struct dfe_sampling_config *sampling_conf)
{
	assert(sampling_conf != NULL);

	u16_t sample_spacing_ns = dfe_get_sample_spacing_ns(sampling_conf->sample_spacing);
	u16_t swich_spacing_ns = dfe_get_switch_spacing_ns(sampling_conf->switch_spacing);
	/* Switch spacing time is a duration between antenna switch so it holds
	 * switch slot and sample slot. Because of that in case of over sampling
	 * we take as valid samples only half of them. In case sampling duration
	 * is greater or equal all samples are valid.
	 * of
	 */
	assert(sample_spacing_ns != 0);

	if (swich_spacing_ns > sample_spacing_ns) {
		return swich_spacing_ns / (sample_spacing_ns * 2);
	} else {
		return sample_spacing_ns / sample_spacing_ns;
	}
}

u16_t get_ref_samples_num(const struct dfe_sampling_config* sampling_conf)
{
	assert(sampling_conf != NULL);

	u16_t sample_spacing_ns = dfe_get_sample_spacing_ref_ns(sampling_conf->sample_spacing_ref);
	return sampling_conf->ref_period_us * DFE_NS(1000) / sample_spacing_ns;
}

enum dfe_sampling_type dfe_get_sampling_type(const struct dfe_sampling_config *sampling_conf)
{
	assert(sampling_conf != NULL);

	/* Please note that values used by radio to set switch spacing and
	 * sample spacing are in reverse order. That means higher value, shorter
	 * duration. E.g. :
	 *  - RADIO_DFECTRL1_TSAMPLESPACING_4us (1UL) [4us]
	    - RADIO_DFECTRL1_TSAMPLESPACING_2us (2UL) [2us]
	 * Because of that logic here is opposite.
	 */
	if (sampling_conf->switch_spacing > sampling_conf->sample_spacing) {
		return DFE_UNDER_SAMPLING;
	} else if (sampling_conf->switch_spacing == sampling_conf->sample_spacing) {
		return DFE_REGULAR_SAMPLING;
	} else {
		return DFE_OVER_SAMPLING;
	}
}

u16_t dfe_delay_before_first_sampl(const struct dfe_sampling_config* sampling_conf)
{
	assert(sampling_conf != NULL);

	u16_t swich_spacing_ns = dfe_get_switch_spacing_ns(sampling_conf->switch_spacing);
	u16_t ref_spacing_ns = dfe_get_sample_spacing_ref_ns(sampling_conf->sample_spacing_ref);

	assert(swich_spacing_ns != 0);
	return (swich_spacing_ns >> 1) + ref_spacing_ns;
}

