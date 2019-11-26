#ifndef __PROTOCOL_H
#define __PROTOCOL_H


#include "protocol_struct.h"


#define PROTOCOL_HEAD		"+AoA"


extern protocol_vector PROTOCOL;


void PROTOCOL_Initialization(if_vector* iface);
void PROTOCOL_Handling(void);

#endif
