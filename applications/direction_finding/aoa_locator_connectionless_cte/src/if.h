/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __IF_H
#define __IF_H

#define IF_BUFFER_SIZE		2048

struct device;

struct device_vector
{
	struct device *dev;
	uint8_t rx_buffer[IF_BUFFER_SIZE];
	uint8_t tx_buffer[IF_BUFFER_SIZE];
	uint16_t rx_index;
	uint16_t tx_index;

	void (*send)(uint8_t *, uint16_t);
};

struct if_data
{
	struct device_vector uart_app;
};

struct if_data* if_initialization(void);

#endif
