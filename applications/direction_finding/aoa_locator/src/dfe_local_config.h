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

/** Macro for unknown antenna index */
#define DFE_ANT_UNKNONW (255)

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

/** @brief Storage for IQ samples collected during CTE sampling
 *
 * Size of a storage is based on settings provided by application configuration
 * (@ref DFE_TOTAL_SLOTS_NUM, @ref DFE_SAMPLES_PER_SLOT_NUM).
 */
struct dfe_iq_data_storage {
	uint8_t slots_num;					//!< number of slots
	uint8_t samples_num;				//!< number of samples in a single slot
	union dfe_iq_f data[DFE_TOTAL_SLOTS_NUM][DFE_SAMPLES_PER_SLOT_NUM];
};

/** @brief Storage for slots data holding IQ samples
 *
 *  * Size of a storage is based on settings provided by application configuration
 * (@ref DFE_TOTAL_SLOTS_NUM).
 */
struct dfe_slot_samples_storage {
	uint8_t slots_num;					//!< number of slots holding IQ samples
	struct dfe_samples data[DFE_TOTAL_SLOTS_NUM];
};

/** @brief Returns instance of DFE sampling configuration data structure.
 *
 * @return instance of sampling configuration
 */
const struct dfe_sampling_config *dfe_get_sampling_config();

/** @brief Returns instance of DFE sampling configuration data structure.
 *
 * @return instance of antenna swtiching configuration
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


/** @brief Maps IQ samples to antennas.
 *
 * Maps IQ samples to antennas used to collect particular samples.
 * Radio provides raw IQ samples without information about time and antenna
 * used to collect them.
 *
 * @note To decrease memory footprint the application is responsible to provide
 * storage for IQ samples and collect them into sets for every sampling slot
 * The library receives only pointers to data and mapping information.
 *
 * @param[out] mapped_data		Storage for mapped data for PDDA evaluation
 * @param[out] iq_storage		Storage for IQ samples
 * @param[out] slots_storage	Storage for sampling slots data
 * @param[in] raw_data		Raw IQ samples received from BLE controller
 * @param[in] sampl_conf	Sampling configuration
 * @param[in] ant_conf		Antenna switching configuration
 */
void dfe_map_iq_samples_to_antennas(struct dfe_mapped_packet *mapped_data,
									struct dfe_iq_data_storage *iq_storage,
									struct dfe_slot_samples_storage *slots_storage,
									const struct dfe_packet *raw_data,
									const struct dfe_sampling_config *sampling_conf,
									const struct dfe_antenna_config *ant_config);

/** @brief Removes samples collected in antenna switch slots
 *
 * If samples spacing is shorter than antenna switch spacing then part of samples
 * are collected during antenna switching slots. That samples may be invalid
 * and introduce error. Because of that they are removed from mapped samples set
 * before following steps of evaluation.
 *
 * @param[out]	data		Storage for mapped IQ samples
 * @param[in]	sampl_conf	Sampling configuration
 */
int remove_samples_from_switch_slot(struct dfe_mapped_packet *data,
				    const struct dfe_sampling_config *sampling_conf);

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

/** @brief Provides number of effective antennas used during antenna switching
 * period.
 *
 * @param[in] sampl_conf	Sampling configuration
 *
 * @return Number of antennas used to collect IQ samples except reference period
 */
uint8_t dfe_get_effective_ant_num(const struct dfe_sampling_config *sampling_conf);

#endif /* SRC_DFE_LOCAL_CONFIG_H_ */
