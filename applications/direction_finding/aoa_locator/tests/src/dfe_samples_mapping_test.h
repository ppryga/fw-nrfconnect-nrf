/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef TESTS_SRC_DFE_SAMPLES_MAPPING_TEST_H_
#define TESTS_SRC_DFE_SAMPLES_MAPPING_TEST_H_

void setup_fixture_prepare_iq_data_and_over_sampling_config();

void test_iq_samples_to_ant_mapping_if_oversampling_configured();
void test_iq_samples_to_ant_mapping_if_ble_compliant_sampling_is_configured();
void test_iq_samples_to_ant_mapping_if_undersampling_is_configured();

void test_remove_samples_from_switching_slots_every_2nd_slot_to_remove();
void test_remove_samples_from_switching_slots_no_slot_to_remove();
void test_remove_samples_from_switching_slots_all_slots_to_remove();

#endif /* TESTS_SRC_DFE_SAMPLES_MAPPING_TEST_H_ */
