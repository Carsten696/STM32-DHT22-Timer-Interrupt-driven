/*
 * DHT22.c
 *
 *  Created on: Mar 17, 2025
 *      Author: C. Frank
 */
#include "stm32h7xx_hal.h"

#ifndef INC_DHT22_H_
#define INC_DHT22_H_

// Fehlercodes
typedef enum {
    DHT22_OK = 0,
    DHT22_NO_DATA = 1,
    DHT22_CHECKSUM_ERR = 2,
    DHT22_TIMEOUT = 3
} DHT22_Status;

typedef enum {
    DHT_IDLE,
    DHT_WAIT_RESPONSE,    // Warten auf erste Antwort
    DHT_WAIT_FIRST_LOW,
    DHT_WAIT_FIRST_HIGH,
    DHT_READING           // Datenbits lesen
} DHT_State_t;


typedef void (*print_func_t)(const char *format, ...);
void DHT22_Init();

//extern TIM_HandleTypeDef htim3;
//extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim6;

void DHT22_Start_Reading(void);
DHT22_Status DHT22_Get_Data(uint16_t *Temperature, uint16_t *Humidity);

#endif /* INC_DHT22_H_ */


