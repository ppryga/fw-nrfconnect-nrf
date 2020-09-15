/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <assert.h>
#include <string.h>

#include <stdlib.h>
#include <kernel.h>
#include <zephyr/types.h>
#include <bluetooth/dfe_data.h>
#include <shell/shell.h>
#include <shell/shell_rtt.h>

#include "if.h"
#include "protocol.h"
#include "dfe_local_config.h"
#include "ble.h"
#include "antenna_test.h"

#define MODULE df_main
#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DF_APP_LOG_LEVEL);

/** @brief Number of ms to wait for a data before printing no data note
 */
#define WAIT_FOR_DATA_BEFORE_PRINT (1000)

/** @brief Shell command handler responsible for antenna testing procedure
 *
 * The command handler accepts single argument that is verbosity.
 * Allowed verbosity levels are in range: @ref TEST_VERBOSITY_LOW to
 * @ref TEST_VERBOSITY_HIGH.
 *
 * @param[in] shell	pointer to shell object
 * @param[in] argc	number of command arguments
 * @param[in] argv	array providing command arguments
 *
 * @retval	0 always return zero
 */
static int start_test(const struct shell *shell, size_t argc, const char **argv)
{
	int verbosity = 1;

	if(argc > 1) {
		int user_verbosity = atoi(argv[1]);
		if (user_verbosity >= TEST_VERBOSITY_LOW &&
			user_verbosity <= TEST_VERBOSITY_HIGH) {
			verbosity = user_verbosity;
		} else {
			shell_print(shell, "Error, verbosity level out of range: %d",
						user_verbosity);
		}
	}

	shell_print(shell, "Antenna matrix test started");

	bool result = run_antenna_test_suite(verbosity);

	if(!result) {
		shell_print(shell, "Antenna matrix test FAILED");
	} else {
		shell_print(shell, "Antenna matrix test SUCCESS");
	}

	return 0;
}

SHELL_CMD_ARG_REGISTER(test, NULL, "Start antennas test. Optionally accepts " \
					   "verbosity level 1-3.", start_test, 0, 1);

/** @brief Main function of the test example.
 *
 */
void main(void)
{
	LOG_INF("Starting AoA Locator CL!");
	/* initialize UART interface to provide I/Q samples */
	struct if_data* iface = if_initialization();

	if (iface == NULL) {
		LOG_ERR("Locator stopped!");
		return;
	}

	int err;

	err = protocol_initialization(iface);
	if (err) {
		LOG_ERR("Locator stopped!");
		return;
	}

	LOG_INF("Initialize Bluetooth");
	ble_initialization();

	 while(1)
	 {
	 	k_sleep(K_MSEC(CONFIG_AOA_LOCATOR_DATA_SEND_WAIT_MS));
	 }

	LOG_INF("nRF antenna tester End");
}
