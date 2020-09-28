/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdint.h>
#include <app_version.h>

__attribute__((section(".version_magic_code"))) uint32_t VERSION_MAGIC[2] = { 0x4E524635, 0x58415050 }; // "NRF5XAPP" string stored as hexes
__attribute__((section(".version_str"))) char VERSION_STR[] = APPLICATION_VERSION_STRING;
__attribute__((section(".build_date_str"))) char DATE_STR[] = __DATE__ " " __TIME__;