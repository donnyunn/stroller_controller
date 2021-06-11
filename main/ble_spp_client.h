#ifndef __BLE_SPP_CLIENT_H
#define __BLE_SPP_CLIENT_H

#include "main.h"

void ble_spp_send(uint8_t * data, uint16_t len);
void ble_spp_client_init(void);

#endif /* __BLE_SPP_CLIENT_H */