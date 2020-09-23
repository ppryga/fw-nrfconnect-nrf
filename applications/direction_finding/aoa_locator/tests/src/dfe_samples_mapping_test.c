/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <ztest.h>

#include "dfe_samples_mapping_test.h"
#include <dfe_local_config.h>
#include <dfe_data_preprocess.h>
#include "configuration_fixtures.h"

/** @brief Storage for artificial DFE data packet.*/
static struct dfe_packet g_test_df_data_packet;
static struct dfe_mapped_packet g_test_df_data_mapped;

/** @brief Test buffer for IQ samples */
static struct dfe_iq_data_storage g_test_iq_storage = {
	.slots_num = DFE_TOTAL_SLOTS_NUM,
	.samples_num = DFE_SAMPLES_PER_SLOT_NUM,
};

/** @brief Test buffer for slots storage */
static struct dfe_slot_samples_storage g_test_slots_storage = {
		.slots_num = DFE_TOTAL_SLOTS_NUM,
};

/** @brief Prepare artificial IQ data as if they were collected
 * by radio when oversampling is enabled.
 *
 * The data are prepared in as if they were collected by radio with following
 * settings:
 * - CTE duration 160us
 * - Reference sampling 250ns
 * - Antenna switch spacing 2us (1us for switch slot, 1us for sample slot)
 * - Samples spacing 250ns
 *
 * Total number of samples in reference period is 8us * 1us/0.250 = 32
 * Antenna switching duration is: 160us - 12us = 148us
 * Total number of samples in antenna switching period is 148 * (1us/0.250) = 592
 */
static void prepare_raw_iq_data_for_max_storage(struct dfe_packet* df_data_packet)
{
	for(int idx = 0; idx < DFE_TOTAL_SAMPLES_NUM; ++idx) {
		df_data_packet->data[idx].iq.i = idx;
		df_data_packet->data[idx].iq.q = 2*idx;
	}
	df_data_packet->hdr.length = DFE_TOTAL_SAMPLES_NUM;
}

void setup_fixture_prepare_iq_data_and_over_sampling_config()
{
	setup_fixture_prepare_over_sampling();
	prepare_raw_iq_data_for_max_storage(&g_test_df_data_packet);
}

void test_iq_samples_to_ant_mapping_if_oversampling_configured(void)
{
	memset(&g_test_df_data_mapped, 0, sizeof(g_test_df_data_mapped));

	struct dfe_sampling_config *sampl_config = get_test_sampl_config();
	struct dfe_antenna_config *ant_conf = get_test_ant_config();

	dfe_map_iq_samples_to_antennas(&g_test_df_data_mapped,
								   &g_test_iq_storage,
								   &g_test_slots_storage,
								   &g_test_df_data_packet,
								   sampl_config, ant_conf);

	/* In case of oversampling a number of samples must be set as taken in
	 * switching slot. Those samples must be discarded before angles evaluation.
	 * Check if appropriate samples are marked as invalid by ant idx set to 0xFF.
	 * If we take 4 samples in a slot, then we have following series:
	 * ANT_X, ANT_X, ANT_X, ANT_X, 0xFF, 0xFF, 0xFF, 0xFF, ANT_X1, ANT_X1...
	 * For configuration where 8 samples is taken in a slot the sequence will
	 * consist of following 8 valid samples, then 8 invalid, etc...
	 */

	uint8_t samples_in_slot = dfe_get_sampling_slot_samples_num(sampl_config);
	zassert_not_equal(samples_in_slot, 0, "There may not be 0 samples in a single slot");

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	zassert_equal(g_test_df_data_mapped.header.length, effective_slots_num,
				  "Wrong length of mapped IQ data");

	for(uint16_t slot_idx = 0, ant_idx=0; slot_idx < effective_slots_num; ++slot_idx) {
		if(slot_idx & 0x1) {
			zassert_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, 0xFF,
						  "Wrong antenna id, it should be 0xFF for discarded samples");
		} else {
			uint8_t ant_num = ant_conf->antennae_switch_idx[ant_idx++];
			if(ant_idx >= ant_conf->antennae_switch_idx_len) {
				ant_idx = 0;
			}
			zassert_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, ant_num,
						  "Wrong antenna idx found. Does not match to configuration");
		}
	}
}

void test_iq_samples_to_ant_mapping_if_ble_compliant_sampling_is_configured(void)
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();
	struct dfe_antenna_config *ant_conf = get_test_ant_config();

	memset(&g_test_df_data_mapped, 0, sizeof(g_test_df_data_mapped));

	dfe_map_iq_samples_to_antennas(&g_test_df_data_mapped,
								   &g_test_iq_storage,
								   &g_test_slots_storage,
								   &g_test_df_data_packet,
								   sampl_config, ant_conf);

	/* In case of sampling that complies BLE specification There are no slots
	 * that should be discarded.
	 */
	uint8_t samples_in_slot = dfe_get_sampling_slot_samples_num(sampl_config);
	zassert_not_equal(samples_in_slot, 0, "There may not be 0 samples in a single slot");

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	zassert_equal(g_test_df_data_mapped.header.length, effective_slots_num,
				  "Wrong length of mapped IQ data");

	for(uint16_t slot_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		uint8_t ant_num = ant_conf->antennae_switch_idx[slot_idx % ant_conf->antennae_switch_idx_len];
		zassert_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, ant_num,
					  "Wrong antenna idx found. Does not match to configuration");
	}
}

void test_iq_samples_to_ant_mapping_if_undersampling_is_configured(void)
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();
	struct dfe_antenna_config *ant_conf = get_test_ant_config();

	memset(&g_test_df_data_mapped, 0, sizeof(g_test_df_data_mapped));

	dfe_map_iq_samples_to_antennas(&g_test_df_data_mapped,
								   &g_test_iq_storage,
								   &g_test_slots_storage,
								   &g_test_df_data_packet,
								   sampl_config, ant_conf);

	/* In case of under-sampling there will be no slot to be discarded (no antenna
	 * with 0xFF. Antenna identifiers will map to every n-th antenna from configuration.
	 * N-th step depends on sample spacing in comparison to switch spacing.
	 */
	uint16_t samples_spacing = dfe_get_sample_spacing_ref_ns(sampl_config->sample_spacing);
	zassert_not_equal(samples_spacing, 0, "Samples spacing may not be zero");

	uint16_t switch_spacing = dfe_get_switch_spacing_ns(sampl_config->switch_spacing);
	zassert_not_equal(switch_spacing, 0, "Switch spacing may not be zero");

	uint16_t ant_step = samples_spacing / switch_spacing;
	zassert_true(ant_step > 0, "Antenna step in undersampling should be greater than 0");

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	zassert_equal(g_test_df_data_mapped.header.length, effective_slots_num,
				  "Wrong length of mapped IQ data");

	for(uint16_t slot_idx = 0, ant_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		uint8_t ant_num = ant_conf->antennae_switch_idx[ant_idx];
		ant_idx += ant_step;
		if(ant_idx >= ant_conf->antennae_switch_idx_len) {
			ant_idx = 0;
		}
		zassert_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, ant_num,
					  "Wrong antenna idx found. Does not match to configuration");
	}
}

void prepare_samples_mapped_to_antennas_every_2nd_slot_to_remove()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	uint8_t samples_num = dfe_get_sampling_slot_samples_num(sampl_config);

	zassert_true((effective_slots_num <= g_test_iq_storage.slots_num), "There is not enough memory for IQ samples for effective slots number");
	zassert_true((effective_slots_num <= g_test_slots_storage.slots_num), "There is not enough memory for slots data storage");
	zassert_true((samples_num <= g_test_iq_storage.samples_num), "Samples number for a slot is greater than memory available in a slot");

	for (uint16_t slot_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		union dfe_iq_f *iq_data = g_test_iq_storage.data[slot_idx];

		uint8_t sample_idx = 0;
		for ( ; sample_idx < samples_num; ++sample_idx) {
			iq_data->i = sample_idx;
			iq_data->q = sample_idx;
		}

		if(slot_idx & 0x1) {
			g_test_slots_storage.data[slot_idx].antenna_id = DFE_ANT_INCORRECT;
		} else {
			g_test_slots_storage.data[slot_idx].antenna_id = slot_idx;
		}
		g_test_slots_storage.data[slot_idx].samples_num = samples_num;
		g_test_slots_storage.data[slot_idx].data = iq_data;
	}

	g_test_df_data_mapped.header.length = effective_slots_num;
	g_test_df_data_mapped.sampl_data = g_test_slots_storage.data;
}

void prepare_samples_mapped_to_antennas_no_slots_to_remove()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	uint8_t samples_num = dfe_get_sampling_slot_samples_num(sampl_config);

	zassert_true((effective_slots_num <= g_test_iq_storage.slots_num), "There is not enough memory for IQ samples for effective slots number");
	zassert_true((effective_slots_num <= g_test_slots_storage.slots_num), "There is not enough memory for slots data storage");
	zassert_true((samples_num <= g_test_iq_storage.samples_num), "Samples number for a slot is greater than memory available in a slot");

	for (uint16_t slot_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		union dfe_iq_f *iq_data = g_test_iq_storage.data[slot_idx];

		uint8_t sample_idx = 0;
		for ( ; sample_idx < samples_num; ++sample_idx) {
			iq_data->i = sample_idx;
			iq_data->q = sample_idx;
		}

		g_test_slots_storage.data[slot_idx].antenna_id = slot_idx;
		g_test_slots_storage.data[slot_idx].samples_num = samples_num;
		g_test_slots_storage.data[slot_idx].data = iq_data;
	}

	g_test_df_data_mapped.header.length = effective_slots_num;
	g_test_df_data_mapped.sampl_data = g_test_slots_storage.data;
}

void prepare_samples_mapped_to_antennas_remove_all_slots()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);
	uint8_t samples_num = dfe_get_sampling_slot_samples_num(sampl_config);

	zassert_true((effective_slots_num <= g_test_iq_storage.slots_num), "There is not enough memory for IQ samples for effective slots number");
	zassert_true((effective_slots_num <= g_test_slots_storage.slots_num), "There is not enough memory for slots data storage");
	zassert_true((samples_num <= g_test_iq_storage.samples_num), "Samples number for a slot is greater than memory available in a slot");

	for (uint16_t slot_idx = 0; slot_idx < effective_slots_num; ++slot_idx) {
		union dfe_iq_f *iq_data = g_test_iq_storage.data[slot_idx];

		uint8_t sample_idx = 0;
		for ( ; sample_idx < samples_num; ++sample_idx) {
			iq_data->i = sample_idx;
			iq_data->q = sample_idx;
		}

		g_test_slots_storage.data[slot_idx].antenna_id = DFE_ANT_INCORRECT;
		g_test_slots_storage.data[slot_idx].samples_num = samples_num;
		g_test_slots_storage.data[slot_idx].data = iq_data;
	}

	g_test_df_data_mapped.header.length = effective_slots_num;
	g_test_df_data_mapped.sampl_data = g_test_slots_storage.data;
}

void test_remove_samples_from_switching_slots_every_2nd_slot_to_remove()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	prepare_samples_mapped_to_antennas_every_2nd_slot_to_remove(&g_test_df_data_packet);
	remove_samples_from_switch_slot(&g_test_df_data_mapped, sampl_config);

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);

	for (int slot_idx = 0; slot_idx < g_test_df_data_mapped.header.length; ++slot_idx) {
		zassert_not_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, DFE_ANT_INCORRECT, "Slot marked as to be discarded found in mapped data");
	}
	effective_slots_num /= 2;
	zassert_equal(g_test_df_data_mapped.header.length, effective_slots_num, "Wrong number of slots found in mapped data");
}

void test_remove_samples_from_switching_slots_no_slot_to_remove()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	prepare_samples_mapped_to_antennas_no_slots_to_remove(&g_test_df_data_packet);
	remove_samples_from_switch_slot(&g_test_df_data_mapped, sampl_config);

	uint16_t effective_slots_num = dfe_get_effective_slots_num(sampl_config);

	for (int slot_idx = 0; slot_idx < g_test_df_data_mapped.header.length; ++slot_idx) {
		zassert_not_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, DFE_ANT_INCORRECT, "Slot marked as to be discarded found in mapped data");
	}

	zassert_equal(g_test_df_data_mapped.header.length, effective_slots_num, "Wrong number of slots found in mapped data");
}

void test_remove_samples_from_switching_slots_all_slots_to_remove()
{
	struct dfe_sampling_config *sampl_config = get_test_sampl_config();

	prepare_samples_mapped_to_antennas_remove_all_slots(&g_test_df_data_packet);
	remove_samples_from_switch_slot(&g_test_df_data_mapped, sampl_config);

	for (int slot_idx = 0; slot_idx < g_test_df_data_mapped.header.length; ++slot_idx) {
		zassert_not_equal(g_test_df_data_mapped.sampl_data[slot_idx].antenna_id, DFE_ANT_INCORRECT, "Slot marked as to be discarded found in mapped data");
	}

	zassert_equal(g_test_df_data_mapped.header.length, 0, "Wrong number of slots found in mapped data");
}
