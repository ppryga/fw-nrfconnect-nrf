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

#include "dfe_local_config.h"
#include "dfe_data_preprocess.h"

void dfe_map_iq_samples_to_antennas(struct dfe_mapped_packet *mapped_data,
									struct dfe_iq_data_storage *iq_storage,
									struct dfe_slot_samples_storage *slots_storage,
									const struct dfe_packet *raw_data,
									const struct dfe_sampling_config *sampling_conf,
									const struct dfe_antenna_config *ant_config)
{
	assert(raw_data != NULL);
	assert(mapped_data != NULL);

	u16_t ref_samples_num = get_ref_samples_num(sampling_conf);
	mapped_data->ref_data.antenna_id = ant_config->ref_ant_idx;

	for(uint16_t idx = 0; idx < ref_samples_num; ++idx) {
		mapped_data->ref_data.data[idx].i = raw_data->data[idx].iq.i;
		mapped_data->ref_data.data[idx].q = raw_data->data[idx].iq.q;
	}
	mapped_data->ref_data.samples_num = ref_samples_num;

	/* Depending on DFE duration, the number of antennas used for sample
	 * may be greater than the number of antennas in configuration.
	 * If there is time left after end of antennas sequence, then radio
	 * starts to use the same antennas again.
	 */
	u16_t effective_slots_num = dfe_get_effective_slots_num(sampling_conf);

	u8_t samples_num = dfe_get_sampling_slot_samples_num(sampling_conf);

	u8_t effective_sample_idx;

	enum dfe_sampling_type sampling_type = dfe_get_sampling_type(sampling_conf);

	/* For regular sampling configuration antennas are assigned
	 * sequentially from antenna configuration with step 1.
	 *
	 * For undersampling radio does not take samples for every antenna,
	 * but switching continues according to antennae sequence configuration.
	 * So we have to assign every n-th antenna to sampling slot.
	 *
	 * For over sampling radio takes samples even in switching slot, so antenna
	 * step is slower than slots number increate by factor of two.
	 */
	u16_t ant_step = 1;
	if(sampling_type == DFE_UNDER_SAMPLING)
	{
		u16_t sampl_spacing = dfe_get_sample_spacing_ns(sampling_conf->sample_spacing);
		u16_t switch_spacing = dfe_get_switch_spacing_ns(sampling_conf->switch_spacing);

		assert(switch_spacing > 0);
		ant_step = sampl_spacing/switch_spacing;
	}

	for(u16_t slot_idx = 0, ant_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		u8_t ant;
		struct dfe_samples *sample = &slots_storage->data[slot_idx];

		if (sampling_type == DFE_OVER_SAMPLING) {
			if (slot_idx & 0x1) {
				ant = DFE_ANT_INCORRECT;
			} else {
				ant = ant_config->antennae_switch_idx[ant_idx];
				ant_idx += ant_step;
				if (ant_idx >= ant_config->antennae_switch_idx_len) {
					ant_idx = 0;
				}
			}
		} else {
			ant = ant_config->antennae_switch_idx[ant_idx];
			ant_idx += ant_step;
			if (ant_idx >= ant_config->antennae_switch_idx_len) {
				ant_idx = 0;
			}
		}

		sample->antenna_id = ant;

		assert(slot_idx <= iq_storage->slots_num);
		assert(samples_num <= iq_storage->samples_num);
		union dfe_iq_f *iq_data = iq_storage->data[slot_idx];

		for(u8_t sample_idx = 0; sample_idx < samples_num; ++sample_idx) {
			effective_sample_idx = ref_samples_num + (slot_idx * samples_num) + sample_idx;
			iq_data[sample_idx].i = raw_data->data[effective_sample_idx].iq.i;
			iq_data[sample_idx].q = raw_data->data[effective_sample_idx].iq.q;
		}
		sample->samples_num = samples_num;
		sample->data = iq_data;
	}

	mapped_data->header.length = effective_slots_num;
	mapped_data->header.frequency = raw_data->hdr.frequency;
	mapped_data->sampl_data = slots_storage->data;
}

int remove_samples_from_switch_slot(struct dfe_mapped_packet *data,
				    const struct dfe_sampling_config *sampling_conf)
{
	assert(data != NULL);
	assert(sampling_conf != NULL);

	u16_t out_idx = 0;
	u16_t in_idx = 0;

	for( ; in_idx < data->header.length; ++in_idx) {
		assert(in_idx >= out_idx);
		const struct dfe_samples *sample_in = &data->sampl_data[in_idx];

		if (sample_in->antenna_id != DFE_ANT_INCORRECT) {
			struct dfe_samples *sample_out = &data->sampl_data[out_idx];

			sample_out->antenna_id = sample_in->antenna_id;
			sample_out->samples_num = sample_in->samples_num;

			for(u8_t sample_idx = 0; sample_idx <= sample_in->samples_num; ++sample_idx) {
				sample_out->data[sample_idx].i = sample_in->data[sample_idx].i;
				sample_out->data[sample_idx].q = sample_in->data[sample_idx].q;
			}
			++out_idx;
		}
	}

	/* update length to new effective antenna number */
	data->header.length = out_idx;

	return 0;
}
