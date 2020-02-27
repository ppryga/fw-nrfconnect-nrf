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

int PROTOCOL_Initialization(struct if_data *iface);
int PROTOCOL_Handling(const struct df_sampling_config *sampl_conf,
		      const struct dfe_mapped_packet *mapped_data);
#endif
