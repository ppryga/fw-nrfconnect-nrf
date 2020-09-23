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

#define DFE_NS(ns)	(ns)
#define DFE_US(us)	(us)

/** Macro for incorrect antenna index */
#define DFE_ANT_INCORRECT (0xFF)

/** @brief DFE antennas switching configuration structure
 */
struct dfe_antenna_config
{
	/** Index of antenna used for reference period */
	u8_t ref_ant_idx;
	/** Index of antenna used at start of CTE and after end of CTE
	* (suggested to use the same antenna as for ref period)
	*/
	u8_t idle_ant_idx;
	 /** The array stores values that should be set to GPIO to enable antenna
	 * with given index. The index of the particular value is the antenna
	 * number. Single bit of the value is set as a value for single GPIO pin.
	 */
	u8_t ant_gpio_pattern[16];
	u8_t ant_gpio_pattern_len;
	/** Array stores indices of antennas to be enabled in particular switch state.
	 * The array does not include reference period antenna and idle antenna.
	 */
	u8_t antennae_switch_idx[48];
	u8_t antennae_switch_idx_len;
};

/** @brief DFE Sampling configuration structure
 */
struct dfe_sampling_config
{
	/** Mode of DFE, currently we support on AoA */
	u8_t dfe_mode;
	/** Start point when CTE is added and start point when switching/sampling
	 * is done: after CRC, during packet payload
	 */
	u8_t start_of_sampl;
	/** Length of AoA/AoD procedure in number of 8[us] */
	u8_t number_of_8us;
	/** @brief Process sampling even if CRC error occurs
	 */
	bool en_sampling_on_crc_error;
	/** Limits start DFE to be triggered by TASKS_DFESTART only */
	bool dfe_trigger_task_only;
	/** Type of samples: I/Q or magnitude/phase */
	u8_t sampling_type;
	/** Interval between samples in REFERENCE period */
	u8_t sample_spacing_ref;
	/** Interval between samples in SWITCH period */
	u8_t sample_spacing;
	/** Time delay relative to beginning of reference period before starting
	 * sampling, number of 16M cycles ?
	 */
	s16_t sample_offset;
	/** Time between consecutive antenna switches in SWITCH period of CTE */
	u16_t switch_spacing;
	/** Time delay after end of CRC before starting switching, number of 16M cycles */
	s16_t switch_offset;
	/** Length of guard period - BT defines that to be 4[us] */
	u8_t guard_period_us;
	/** Length of reference period - BT defines that to be 8[us] */
	u8_t ref_period_us;
};

/** @brief DFE sampling type.
 *
 * Sampling of received CTE may be configured to produce:
 * - regular sampling, when antenna switch spacing is equal to samples spacing
 * - over sampling, when antenna switch spacing is longer than samples spacing
 * - under sampling, when antenna switch spacing is shorter than samples spacing
 *
 * @note This applies for samples taken in antenna switching period of CTE receive.
 */
enum dfe_sampling_type {
	DFE_REGULAR_SAMPLING = 0,
	DFE_OVER_SAMPLING,
	DFE_UNDER_SAMPLING
};


/** @brief Returns instance of DFE sampling configuration data structure.
 *
 * @return instance of sampling configuration
 */
const struct dfe_sampling_config *dfe_get_sampling_config();

/** @brief Returns instance of DFE sampling configuration data structure.
 *
 * @return instance of antenna switching configuration
 */
const struct dfe_antenna_config *dfe_get_antenna_config();

/** @brief Returns configuration of GPIOs that should be used to drive antenna
 * switching by DFE module.
 *
 * @return pointer to GPIOs array
 */
const struct dfe_ant_gpio* dfe_get_ant_gpios_config();

/** @brief Returns length of array returned by dfe_get_ant_gpios_config
 *
 * @return length of array of GPIOs to drive antenna switches
 */
u8_t dfe_get_ant_gpios_config_len();

/** @brief Initializes DFE in Bluetooth controller
 *
 * @param[in] sampl_conf	Sampling configuration
 * @param[in] ant_conf		Antenna switching configuration
 * @param[in] ant_gpio		Array of GPIOs to use to drive antenna switches
 * @param[in] ant_gpio_len	Length of @p ant_gpio array
 *
 * @return Returns zero if DFE is initialized successfully, non zero value in
 * 		   other case.
 */
int dfe_init(const struct dfe_sampling_config *sampl_conf,
	     const struct dfe_antenna_config *ant_conf,
	     const struct dfe_ant_gpio *ant_gpio, u8_t ant_gpio_len);

/** @brief Provide information about sampling type.
 *
 * Function provides information about type of a sampling that is
 * configured for DFE. To check possible sampling types see @ref dfe_sampling_type.
 *
 * @param[in] sampling_conf	Sampling configuration
 *
 * @return dfe_sampling_type Configured type of a sampling
 */
enum dfe_sampling_type dfe_get_sampling_type(const struct dfe_sampling_config *sampling_conf);

/** @brief Evaluates delay between last reference sample and first sample in
 * antenna switching period.
 *
 * @param[in]	sampling_conf	Sampling configuration
 *
 * @return	Time delay in [ns]
 */
u16_t dfe_delay_before_first_sampl(const struct dfe_sampling_config* sampling_conf);

/** @brief Converts sample spacing settings value to nanoseconds.
 *
 * @param[in] sampling	Value of sampling setting
 *
 * @return Sample spacing in [ns]
 */
u16_t dfe_get_sample_spacing_ns(u8_t sampling);

/** @brief Converts reference sample spacing settings value to nanoseconds.
 *
 * @param[in] sampling	Value of reference sampling setting
 *
 * @return Sample spacing in [ns]
 */
u16_t dfe_get_sample_spacing_ref_ns(u8_t sampling);

/** @brief Converts antenna switch spacing settings value to nanoseconds.
 *
 * @param[in] spacing	Value of antenna switching setting
 *
 * @return Sample spacing in [ns]
 */
u16_t dfe_get_switch_spacing_ns(u8_t spacing);

/** @brief Provides number of samples collected in single sampling slot
 *
 * @param[in] sampl_conf	Sampling configuration
 *
 * @return Number of samples collected in sampling slot
 */
u16_t dfe_get_sampling_slot_samples_num(const struct dfe_sampling_config *sampling_conf);

/** @brief Provides effective number of slots during antenna switching period.
 *
 * @param[in] sampl_conf	Sampling configuration
 *
 * @return Number of slots used to collect IQ samples except reference period
 */
uint16_t dfe_get_effective_slots_num(const struct dfe_sampling_config *sampling_conf);

/** @brief Evaluates number of samples collected in reference period
 *
 * @param[in] sampling_conf	Sampling configuration
 *
 * @return Number of samples
 */
u16_t get_ref_samples_num(const struct dfe_sampling_config* sampling_conf);

#endif /* SRC_DFE_LOCAL_CONFIG_H_ */
