#include "packet.h"

void packet_encoding(packet_t type, uint8_t* packet, uint32_t btn, int x, int y)
{
    static uint8_t cnt = 0;
    // static uint16_t button_map = 0x0000;

    switch (type) {
        case PACKET_LIVE_CHECK:
            packet[0] = 0x00;
            packet[1] = 0x00;
            packet[2] = cnt++;
            packet[3] = 0;
        break;
        case PACKET_USER_OPERATION:
            packet[0] = 0x01;
            packet[1] = 0x00;
            packet[2] = cnt++;
            packet[3] = 6;
            switch (btn) {
                case CONFIG_JOYSTICK_BUTTON:
                packet[5] = 1;
                break;
                case CONFIG_PAIRING_BUTTON:
                break;
                case CONFIG_BRAKE_BUTTON:
                packet[4] = 1;
                break;
                default:
                packet[4] = 0;
                packet[5] = 0;
                break;
            }
            packet[6] = (x & 0x00ff) >> 0;
            packet[7] = (x & 0xff00) >> 8;
            packet[8] = (y & 0x00ff) >> 0;
            packet[9] = (y & 0xff00) >> 8;
        break;
    }
}