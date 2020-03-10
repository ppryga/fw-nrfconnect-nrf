/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#ifndef SRC_DFE_LOCAL_CONFIG_H_
#define SRC_DFE_LOCAL_CONFIG_H_

#include <bluetooth/dfe_config.h>
#include <bluetooth/dfe_data.h>
#include "dfe_samples_data.h"

#define K_NSEC(ns)	(ns)
#define DFE_ANT_UNKNONW (255)

struct dfe_antenna_config{
	/* Index of antenna used for reference period */
	u8_t ref_ant_idx;
	/* Index of antenna used at start of CTE and after end of CTE
	 * (suggested to use the same antenna as for ref period) */
	u8_t idle_ant_idx;
	/* The array stores values that should be set to GPIO to enable antenna
	 * with given index. The index of the particular value is the antenna
	 * number. Single bit of the value is set as a value for signle GPIO pin. */
	u8_t ant_gpio_pattern[16];
	u8_t ant_gpio_pattern_len;
	/* Array stores indices of antennas to be enabled in particular switch state.
	 * The array does not include reference period antenna and idle antenna. */
	u8_t antennae_switch_idx[48];
	u8_t antennae_switch_idx_len;
	/* GPIOS used to switch antennae. */
	u32_t gpio[8];
};


struct dfe_sampling_config {
	/* Mode of DFE, currently we support on AoA */
	u8_t dfe_mode;
	/* Start point when CTE is added and start point when switching/sampling
	 * is done: after CRC, during packet payload */
	u8_t start_of_sampl;
	/* Length of AoA/AoD procedure in number of 8[us] */
	u8_t number_of_8us;
	/* Process sampling even if CRC error occurs */
	bool en_sampling_on_crc_error;
	/* Limits start DFE to be triggered by TASKS_DFESTART only */
	bool dfe_trigger_task_only;
	/* Type of samples: I/Q or magnitude/phase */
	u8_t sampling_type;
	/* Interval between samples in REFERENCE period */
	u8_t sample_spacing_ref;
	/* Interval between samples in SWITCH period */
	u8_t sample_spacing;
	/* Time delay relative to beginning of reference period before starting
	 * sampling, number of 16M cycles ? */
	s16_t sample_offset;
	/* Time between consecutive antenna switches in SWITCH period of CTE */
	u16_t switch_spacing;
	/* Time delay after end of CRC before starting switching, number of 16M cycles */
	s16_t switch_offset;
	/* length of guard period - BT defines that to be 4[us] */
	u8_t guard_period_us;
	/* length of reference period - BT defines that to be 8[us] */
	u8_t ref_period_us;
};

const struct dfe_sampling_config *dfe_get_sampling_config();
const struct dfe_antenna_config *dfe_get_antenna_config();
const struct dfe_ant_gpio* dfe_get_ant_gpios_config();
u8_t dfe_get_ant_gpios_config_len();

int dfe_init(const struct dfe_sampling_config *sampl_conf,
	     const struct dfe_antenna_config *ant_conf,
	     const struct dfe_ant_gpio *ant_gpio, u8_t ant_gpio_len);

int dfe_map_iq_samples_to_antennas(struct dfe_mapped_packet *mapped_data,
				  const struct df_packet *raw_data,
				  const struct dfe_sampling_config *sampling_conf,
				  const struct dfe_antenna_config *ant_config);
int remove_samples_from_switch_slot(struct dfe_mapped_packet *data_out,
				    /*const struct dfe_mapped_packet *in_data,*/
				    const struct dfe_sampling_config *sampling_conf);

u16_t dfe_delay_before_first_sampl(const struct dfe_sampling_config* sampling_conf);
u16_t dfe_get_sample_spacing_ns(u8_t sampling);
u16_t dfe_get_sample_spacing_ref_ns(u8_t sampling);
u16_t dfe_get_switch_spacing_ns(u8_t spacing);
u16_t dfe_get_sampling_slot_samples_num(const struct dfe_sampling_config *sampling_conf);
uint8_t dfe_get_effective_ant_num(const struct dfe_sampling_config *sampling_conf);

#endif /* SRC_DFE_LOCAL_CONFIG_H_ */
