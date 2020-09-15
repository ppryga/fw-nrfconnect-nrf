/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */


#ifndef __BLE_H
#define __BLE_H

/** @brief Initialize Bluetooth stack and starts scanning
 */
int ble_initialization();

/** @brief Starts Bluetooth scanning
 *
 * @return 0 if success, otherwise a (negative) error code.
 */
int bt_start_scanning();

/** @brief Stops Bluetooth scanning
 *
 * @return 0 if success, otherwise a (negative) error code.
 */
int bt_stop_scanning();

#endif
