/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <zephyr/types.h>
#include <bluetooth/dfe_data.h>
#include "dfe_local_config.h"
#include "if.h"

#define PROTOCOL_HEAD				"+AoA"
#define PROTOCOL_BUFFER_SIZE			64
#define PROTOCOL_STRING_BUFFER_SIZE		10240

struct protocol_data
{
	struct device_vector *uart;
	char string_packet[PROTOCOL_STRING_BUFFER_SIZE];
};

int protocol_initialization(struct if_data *iface);
int protocol_handling(const struct df_sampling_config *sampl_conf,
		      const struct dfe_mapped_packet *mapped_data);
#endif
