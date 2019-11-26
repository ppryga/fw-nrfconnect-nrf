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


protocol_vector PROTOCOL;



static uint16_t protocol_convert_to_string(protocol_tx_packet_struct *packet, char *buffer, uint16_t length);


void PROTOCOL_Initialization(if_vector* iface)
{
	protocol_vector *protocol = &PROTOCOL;
	protocol->uart = &iface->uart_app;
	printk("PROTOCOL_Initialization: %p %p\r\n", &iface->uart_app, iface->uart_app.send);
}


void PROTOCOL_Handling(void)
{
	protocol_vector *protocol = &PROTOCOL;

	uint16_t length = 0;

	switch (protocol->tx_status)
	{
		case PROTOCOL_REQUEST_STRING_SEND:
			length = protocol_convert_to_string(&protocol->tx_packet, protocol->string_packet, PROTOCOL_STRING_BUFFER_SIZE);
			//printk("PROTOCOL_Handling send: %p\r\n",protocol->uart->send);
			protocol->uart->send(protocol->string_packet, length);
			protocol->tx_status = PROTOCOL_PENDING_FOR_DATA;
			break;
		default:
			break;
	}
}

static uint16_t protocol_convert_to_string(protocol_tx_packet_struct *packet, char *buffer, uint16_t length)
{
	printk("DF_BEGIN\r\n");
	uint16_t strlen = sprintf(buffer, "DF_BEGIN\r\n");

	strlen += sprintf(&buffer[strlen], "SW:%d\r\n", (int)packet->df_sw);
	strlen += sprintf(&buffer[strlen], "RR:%d\r\n", (int)packet->df_r);
	strlen += sprintf(&buffer[strlen], "SS:%d\r\n", (int)packet->df_s);
	strlen += sprintf(&buffer[strlen], "FR:%d\r\n", (int)packet->frequency);

	strlen += sprintf(&buffer[strlen], "ME:%d\r\n", (int)packet->pdda_elevation);
	strlen += sprintf(&buffer[strlen], "MA:%d\r\n", (int)packet->pdda_azimuth);
	strlen += sprintf(&buffer[strlen], "KE:%d\r\n", (int)packet->kalman_elevation);
	strlen += sprintf(&buffer[strlen], "KA:%d\r\n", (int)packet->kalman_azimuth);

	for(uint16_t i=0; i<packet->iq_length; i++)
	{
		strlen += sprintf(&buffer[strlen], "IQ:%d,%d,%d,%d,%d\r\n", i, (int)sample_time[i], (int)sample_antenna_id[i], (int)packet->iq[i*2], (int)packet->iq[i*2+1]);
	}

	strlen += sprintf(&buffer[strlen], "DF_END\r\n");

	return strlen;
}


