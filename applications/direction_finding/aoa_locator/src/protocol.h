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
#include "aoa.h"

#define PROTOCOL_HEAD				"+AoA"
#define PROTOCOL_BUFFER_SIZE			64
#define PROTOCOL_STRING_BUFFER_SIZE		10240

struct protocol_data
{
	struct device_vector *uart;
	char string_packet[PROTOCOL_STRING_BUFFER_SIZE];
	size_t stored_data_len;
};

int data_transfer_init(struct if_data* iface);
void data_tranfer_prepare_header();
void data_transfer_prepare_samples(const struct dfe_sampling_config* sampl_conf,
				  const struct dfe_mapped_packet *mapped_data);
void data_tranfer_prepare_results(const struct dfe_sampling_config* sampl_conf,
				const struct aoa_results *result);
void data_tranfer_prepare_footer();
void data_tranfer_send();

#endif
