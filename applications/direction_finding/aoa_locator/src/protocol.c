#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <uart.h>
#include <misc/printk.h>

#include "ll_sw/df_config.h"

#include "protocol.h"
#include "common.h"
#include "if.h"

static struct protocol_data g_protocol_data;

static uint16_t protocol_convert_to_string(const struct aoa_configuration *config,
					   const struct df_packet *df_packet,
					   const struct aoa_results *results, char *buffer,
					   uint16_t length);

int PROTOCOL_Initialization(struct if_data* iface)
{
	if (iface == NULL) {
		return -EINVAL;
	}

	g_protocol_data.uart = &iface->uart_app;
	return 0;
}

int PROTOCOL_Handling(const struct aoa_configuration *config, const struct df_packet *df_data_packet,
		      const struct aoa_results *results)
{
	if (config == NULL || df_data_packet == NULL || results == NULL) {
		return -EINVAL;
	}

	uint16_t length = 0;

	length = protocol_convert_to_string(config, df_data_packet, results, g_protocol_data.string_packet, PROTOCOL_STRING_BUFFER_SIZE);
	//printk("PROTOCOL_Handling %s\r\n",protocol->string_packet);
	g_protocol_data.uart->send(g_protocol_data.string_packet, length);

	return 0;
}

static uint16_t protocol_convert_to_string(const struct aoa_configuration *config,
					   const struct df_packet *df_data_packet,
					   const struct aoa_results *results,
					   char *buffer, uint16_t length)
{
	printk("DF_BEGIN\r\n");
	uint16_t strlen = sprintf(buffer, "DF_BEGIN\r\n");

	strlen += sprintf(&buffer[strlen], "SW:%d\r\n", (int)config->df_sw);
	strlen += sprintf(&buffer[strlen], "RR:%d\r\n", (int)config->df_r);
	strlen += sprintf(&buffer[strlen], "SS:%d\r\n", (int)config->df_s);
	strlen += sprintf(&buffer[strlen], "FR:%d\r\n", (int)results->frequency);

	strlen += sprintf(&buffer[strlen], "ME:%d\r\n", (int)results->raw_result.elevation);
	strlen += sprintf(&buffer[strlen], "MA:%d\r\n", (int)results->raw_result.azimuth);
	strlen += sprintf(&buffer[strlen], "KE:%d\r\n", (int)results->filtered_result.elevation);
	strlen += sprintf(&buffer[strlen], "KA:%d\r\n", (int)results->filtered_result.azimuth);

	for(uint16_t i=0; i<df_data_packet->hdr.length; i++)
	{
		strlen += sprintf(&buffer[strlen], "IQ:%d,%d,%d,%d,%d\r\n", i,
				 (int)sample_time[i], (int)sample_antenna_id[i],
				 (int)df_data_packet->data[i].iq.q, (int)df_data_packet->data[i].iq.i);
	}

	strlen += sprintf(&buffer[strlen], "DF_END\r\n");

	return strlen;
}


