#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "main.h"

typedef struct {
    int x;
    int y;

    xQueueHandle pos_queue;
    xQueueHandle btn_queue;
    SemaphoreHandle_t xSemaphore;
} joystick_t;

void joystick_init(joystick_t * joystick);

#endif /* __JOYSTICK_H */