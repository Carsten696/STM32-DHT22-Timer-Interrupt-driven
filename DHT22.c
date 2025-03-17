/*
 * DHT22.c
 *
 *  Created on: Mar 17, 2025
 *      Author: C. Frank
 */

#include "DHT22.h"
#include <string.h>
#include <stdarg.h>
//static GPIO_TypeDef* wire_Port;
//static uint16_t wire_Pin;
GPIO_InitTypeDef GPIO_InitStruct;
uint8_t Hum_byte1, Hum_byte2, Temp_byte1, Temp_byte2, err;
uint16_t Sum;

// ------- NEW ---------
// Private Variablen
static DHT_State_t dht_state = DHT_IDLE;
//static uint32_t sample_counter = 0;
static uint8_t bit_counter = 0;
//static uint8_t bit_counter2 = 0;
static uint8_t byte_counter = 0;
static uint8_t current_byte = 0;
static uint8_t last_level = 1;
//static uint32_t high_time = 0;
static uint8_t data_buffer[5];
static uint8_t data_ready = 0;
static uint32_t bit_time = 0;  // Neue Variable für Bit-Timing

#define DEBUG_EVENTS_MAX 50

typedef enum {
    EVENT_BIT,
    EVENT_BYTE,
    EVENT_COMPLETE
} DebugEventType;

typedef struct {
    DebugEventType type;
    uint8_t byte_num;
    uint8_t value;
    uint32_t timestamp;
} DebugEvent;

// ---------------------

static void Timer_Init(void)
{
    // Timer-Interrupt starten
    HAL_TIM_Base_Start_IT(&htim3);  // Direkt die externe htim3 verwenden
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM3)
    {
        uint8_t current_level = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_3);

        switch(dht_state)
        {
            case DHT_WAIT_RESPONSE:
                if(current_level == 0)  // Erste Antwort vom DHT22
                {
                    // Markiere Start der Antwort
                    /*HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
                    for(volatile int i = 0; i < 100; i++);
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);*/

                    dht_state = DHT_WAIT_FIRST_LOW;
                    bit_time = 0;
                }
                break;

            case DHT_WAIT_FIRST_LOW:
                bit_time++;
                if(bit_time >= 8 && current_level == 1)  // Warten auf Ende der LOW-Phase
                {
                    // Markiere Ende der ersten LOW-Phase
                    /*HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
                    for(volatile int i = 0; i < 100; i++);
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);*/

                    dht_state = DHT_WAIT_FIRST_HIGH;
                    bit_time = 0;
                }
                break;

            case DHT_WAIT_FIRST_HIGH:
                bit_time++;
                if(bit_time >= 8 && current_level == 0)  // Warten auf Ende der HIGH-Phase
                {
                    // Markiere Start der Datenübertragung
                    /*HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
                    for(volatile int i = 0; i < 100; i++);
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);*/

                    dht_state = DHT_READING;
                    bit_time = 0;
                    bit_counter = 0;
                    byte_counter = 0;
                    current_byte = 0;
                }
                break;

            case DHT_READING:
                if(current_level != last_level)  // Flankenänderung
                {
                    if(current_level == 1)  // Steigende Flanke - Start der HIGH-Phase
                    {
                        bit_time = 0;  // Start der Zeitmessung für HIGH-Phase
                    }
                    else  // Fallende Flanke - Ende der HIGH-Phase
                    {
                        current_byte = (current_byte << 1);
                        if(bit_time > 5)  // >70µs = 1
                        {
                            current_byte |= 1;
                            // Zwei Impulse für eine 1
                            /*for(int i = 0; i < 2; i++) {
                                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
                                for(volatile int j = 0; j < 20; j++);
                                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
                                for(volatile int j = 0; j < 20; j++);
                            }*/
                        }
                        else
                        {
                            // Ein Impuls für eine 0
                        	/*HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
                            for(volatile int j = 0; j < 20; j++);
                            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);*/
                        }

                        bit_counter++;
                        if(bit_counter == 8)
                        {
                            data_buffer[byte_counter] = current_byte;
                            byte_counter++;
                            bit_counter = 0;
                            current_byte = 0;

                            if(byte_counter >= 5)
                            {
                                data_ready = 1;
                                dht_state = DHT_IDLE;
                            }
                        }
                    }
                }
                else if(current_level == 1)
                {
                    bit_time++;  // Nur während HIGH-Phase zählen
                }
                break;

            default:
                break;
        }

        last_level = current_level;
    }
}

// Private Funktionen für GPIO-Konfiguration
static void set_gpio_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

static void set_gpio_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void DHT22_Init(void) {
    // GPIO Clocks aktivieren
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // DHT22 Pin als Output konfigurieren
    set_gpio_output();
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);

    // Debug-Pin konfigurieren
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // Timer für 10µs Sampling initialisieren
    HAL_TIM_Base_Start_IT(&htim3);

    dht_state = DHT_IDLE;
}


void DHT22_Start_Reading(void) {
    if(dht_state == DHT_IDLE) {
        // Start-Signal senden
        set_gpio_output();
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_Delay(1);  // 1ms LOW
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3, GPIO_PIN_SET);

        // Auf Input umschalten
        set_gpio_input();
        dht_state = DHT_WAIT_RESPONSE;  // Neuer Anfangszustand
        bit_time = 0;
    }
}

DHT22_Status DHT22_Get_Data(uint16_t *Temperature, uint16_t *Humidity) {
    if(!data_ready) {
        return DHT22_NO_DATA;
    }

    uint8_t sum = data_buffer[0] + data_buffer[1] +
                  data_buffer[2] + data_buffer[3];

    if(sum != data_buffer[4]) {
        return DHT22_CHECKSUM_ERR;
    }

    *Humidity = (data_buffer[0] << 8) | data_buffer[1];
    *Temperature = (data_buffer[2] << 8) | data_buffer[3];

    data_ready = 0;
    return DHT22_OK;
}

