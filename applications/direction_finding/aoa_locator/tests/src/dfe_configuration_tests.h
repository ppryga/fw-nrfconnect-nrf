/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#ifndef TESTS_SRC_DFE_CONFIGURATION_TESTS_H_
#define TESTS_SRC_DFE_CONFIGURATION_TESTS_H_

void test_sampling_type_when_oversampling_configured();
void test_sampling_type_when_bt_1us_slot_configured();
void test_sampling_type_when__bt_2us_slot_configured();
void test_sampling_type_when_undersampling_configured_ommit_2nd_slot();
void test_sampling_type_when_undersampling_configured_ommit_4nd_slot();

void test_calculation_of_effective_slots_number_for_oversampling();
void test_calculation_of_effective_slots_number_for_ble_1us_slot();
void test_calculation_of_effective_slots_number_for_ble_2us_slot();
void test_calculation_of_effective_slots_number_for_under_sampling();

#endif /* TESTS_SRC_DFE_CONFIGURATION_TESTS_H_ */
