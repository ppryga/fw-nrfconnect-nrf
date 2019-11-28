#ifndef __IF_H
#define __IF_H


#define IF_BUFFER_SIZE		2048

struct device;

typedef struct
{
	struct device *dev;
	uint8_t rx_buffer[IF_BUFFER_SIZE];
	uint8_t tx_buffer[IF_BUFFER_SIZE];
	uint16_t rx_index;
	uint16_t tx_index;

	void (*send)(uint8_t *, uint16_t);
} device_vector;


typedef struct if_data_t
{
	device_vector uart_app;
} if_data;

if_data* IF_Initialization(void);

#endif
