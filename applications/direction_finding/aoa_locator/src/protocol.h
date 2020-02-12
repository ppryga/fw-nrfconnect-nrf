#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <zephyr/types.h>
#include "if.h"
#include "aoa.h"
#include "ll_sw/df_data.h"

#define PROTOCOL_HEAD				"+AoA"
#define PROTOCOL_BUFFER_SIZE			64
#define PROTOCOL_STRING_BUFFER_SIZE		10240

struct protocol_data
{
	struct device_vector *uart;
	char string_packet[PROTOCOL_STRING_BUFFER_SIZE];
};

int PROTOCOL_Initialization(struct if_data *iface);
int PROTOCOL_Handling(const struct aoa_configuration *config,
		      const struct df_packet *df_packet,
		      const struct aoa_results *results);

#endif
