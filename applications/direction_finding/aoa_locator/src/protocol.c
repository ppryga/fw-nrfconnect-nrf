/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/printk.h>

#include "protocol.h"
#include "if.h"

#define SAMPLING_TIME_UNIT (125) //!< smallest possible time between samples [ns]

static struct protocol_data g_protocol_data;


int data_transfer_init(struct if_data* iface)
{
	if (iface == NULL) {
		printk("[PROTOCOL] - iface is NULL, cannot initialize\r\n");
		return -EINVAL;
	}

	g_protocol_data.uart = iface;
	return 0;
}

void data_tranfer_prepare_header()
{
	g_protocol_data.stored_data_len = sprintf(g_protocol_data.string_packet,
						  "DF_BEGIN\r\n");
}

void data_transfer_prepare_samples(const struct dfe_sampling_config* sampl_conf,
										  const struct dfe_mapped_packet *mapped_data)
{
	assert(sampl_conf != NULL);
	assert(mapped_data != NULL);

	u16_t strlen = 0;
	char *buffer = &g_protocol_data.string_packet[g_protocol_data.stored_data_len];

	u16_t time_u = dfe_get_sample_spacing_ref_ns(sampl_conf->sample_spacing_ref) / SAMPLING_TIME_UNIT;
	u16_t ref_idx;

	for(ref_idx = 0; ref_idx < mapped_data->ref_data.samples_num; ++ref_idx)
	{
		strlen += sprintf(&buffer[strlen], "IQ:%d,%d,%d,%d,%d\r\n", ref_idx,
				time_u * ref_idx,
				 (int)mapped_data->ref_data.antenna_id,
				 (int)mapped_data->ref_data.data[ref_idx].q,
				 (int)mapped_data->ref_data.data[ref_idx].i);
	}
	/* compute delay  between last sample in reference period and first sample
	 * in antenna switching period.
	 */
	u16_t delay =  dfe_delay_before_first_sampl(sampl_conf) / SAMPLING_TIME_UNIT;
	delay += (time_u * (ref_idx-1));
	time_u = dfe_get_sample_spacing_ns(sampl_conf->sample_spacing) / SAMPLING_TIME_UNIT;

	for(u16_t idx=0; idx<mapped_data->header.length; ++idx)
	{
		const struct dfe_samples* sampl_data = &mapped_data->sampl_data[idx];

		for(u16_t jdx = 0; jdx < sampl_data->samples_num; ++jdx)
		{
			u16_t idx_offset = (sampl_data->samples_num * idx) + jdx;

			strlen += sprintf(&buffer[strlen], "IQ:%d,%d,%d,%d,%d\r\n", ref_idx + idx_offset,
					delay + (idx_offset) * time_u,
					 (int)sampl_data->antenna_id,
					 (int)sampl_data->data[jdx].q,
					 (int)sampl_data->data[jdx].i);
		}
	}

	g_protocol_data.stored_data_len += strlen;
}

void data_tranfer_prepare_results(const struct dfe_sampling_config* sampl_conf,
				const struct aoa_results *result)
{
	assert(sampl_conf != NULL);
	assert(result != NULL);

	u16_t strlen = 0;
	char *buffer = &g_protocol_data.string_packet[g_protocol_data.stored_data_len];

	strlen += sprintf(&buffer[strlen], "SW:%d\r\n", (int)sampl_conf->switch_spacing);
	strlen += sprintf(&buffer[strlen], "RR:%d\r\n", (int)sampl_conf->sample_spacing_ref);
	strlen += sprintf(&buffer[strlen], "SS:%d\r\n", (int)sampl_conf->sample_spacing);
	strlen += sprintf(&buffer[strlen], "FR:%d\r\n", (int)result->frequency);

	strlen += sprintf(&buffer[strlen], "ME:%d\r\n", (int)result->raw_result.elevation);
	strlen += sprintf(&buffer[strlen], "MA:%d\r\n", (int)result->raw_result.azimuth);
	strlen += sprintf(&buffer[strlen], "KE:%d\r\n", (int)result->filtered_result.elevation);
	strlen += sprintf(&buffer[strlen], "KA:%d\r\n", (int)result->filtered_result.azimuth);

	g_protocol_data.stored_data_len += strlen;
}

void data_tranfer_prepare_footer()
{
	u16_t strlen = 0;
	char *buffer = &g_protocol_data.string_packet[g_protocol_data.stored_data_len];
	g_protocol_data.stored_data_len += sprintf(&buffer[strlen], "DF_END\r\n");
}

void data_tranfer_send()
{
	g_protocol_data.uart->send(g_protocol_data.string_packet,
				   g_protocol_data.stored_data_len);
}

