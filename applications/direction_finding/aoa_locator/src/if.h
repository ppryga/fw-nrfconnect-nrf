#ifndef __IF_H
#define __IF_H


#include "if_struct.h"


extern if_vector IF;


if_vector* IF_Initialization(void);
void uart_send(uint8_t *buffer, uint16_t length);

#endif
