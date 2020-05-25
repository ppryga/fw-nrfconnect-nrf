/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef DF_ANTENNA_TEST_H_
#define DF_ANTENNA_TEST_H_

/** @brief Verbosity level of test output */
enum test_verbosity_level {
	/** Log most important information as success, fail */
	TEST_VERBOSITY_LOW = 1,
	/** Log information about test substeps evaluation results for */
	TEST_VERBOSITY_MED,
	/** Log detailed information for every tested IQ sample */
	TEST_VERBOSITY_HIGH
};

/* @brief Run antenna tests suite
 *
 * @param[in] verbosity level of test log verbosity
 *
 * @return 0 if test successful, otherwise a (negative) error code.
 */
int run_antenna_test_suite(enum test_verbosity_level verbosity);

#endif /* DF_ANTENNA_TEST_H_ */
