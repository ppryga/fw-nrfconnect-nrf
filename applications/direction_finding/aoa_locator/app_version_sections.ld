/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

	. = ALIGN(4);
	_version_magic_code_start = .;
	KEEP(*(.version_magic_code));
	_version_magic_code_end  = .;
	. = ALIGN(4);
	_version_info_start = .;
	KEEP(*(.version_str));
	KEEP(*(.build_date_str));
	_version_info_end  = .;
	. = ALIGN(4);
	_version_info_size = ABSOLUTE(_version_info_end - _version_info_start);

