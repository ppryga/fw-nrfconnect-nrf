/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef SRC_DFE_DATA_PREPROCESS_H_
#define SRC_DFE_DATA_PREPROCESS_H_

#include <bluetooth/dfe_data.h>
#include "dfe_samples_data.h"

/** @brief Storage for IQ samples collected during CTE sampling
 *
 * Size of a storage is based on settings provided by application configuration
 * (@ref DFE_TOTAL_SLOTS_NUM, @ref DFE_SAMPLES_PER_SLOT_NUM).
 */
struct dfe_iq_data_storage {
	uint16_t slots_num;					//!< number of slots
	uint8_t samples_num;				//!< number of samples in a single slot
	union dfe_iq_f data[DFE_TOTAL_SLOTS_NUM][DFE_SAMPLES_PER_SLOT_NUM];
};

/** @brief Storage for slots data holding IQ samples
 *
 *  * Size of a storage is based on settings provided by application configuration
 * (@ref DFE_TOTAL_SLOTS_NUM).
 */
struct dfe_slot_samples_storage {
	uint16_t slots_num;					//!< number of slots holding IQ samples
	struct dfe_samples data[DFE_TOTAL_SLOTS_NUM];
};

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

#endif /* SRC_DFE_DATA_PREPROCESS_H_ */
