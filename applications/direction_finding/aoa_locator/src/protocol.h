#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <zephyr/types.h>
#include "if.h"
#include "aoa.h"

#define PROTOCOL_HEAD					"+AoA"
#define PROTOCOL_BUFFER_SIZE			64
#define PROTOCOL_STRING_BUFFER_SIZE		10240

typedef struct protocol_data_t
{
	device_vector *uart;
	char string_packet[PROTOCOL_STRING_BUFFER_SIZE];
} protocol_data;

int PROTOCOL_Initialization(if_data *iface);
int PROTOCOL_Handling(const aoa_configuration *config,
		       const struct df_packet *df_packet,
		       const aoa_results *results);

#endif
