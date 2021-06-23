#ifndef __PACKET_H
#define __PACKET_H

#include "main.h"

typedef enum {
    PACKET_LIVE_CHECK = 0,
    PACKET_USER_OPERATION,
} packet_t;

void packet_encoding(packet_t type, uint8_t* packet, uint32_t btn, int x, int y);

#endif /* __PACKET_H */