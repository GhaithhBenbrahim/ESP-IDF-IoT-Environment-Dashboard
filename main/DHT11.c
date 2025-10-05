/*
 * DHT11.c
 *
 *  Created on: 2 oct. 2025
 *      Author: benbr
 */

/*------------------------------------------------------------------------------
    DHT11 temperature & humidity sensor driver for ESP32
    Built by BenBr (based on open-source DHT drivers)

    Notes:
    - DHT11 provides integer values (no decimal fractions).
    - Humidity: 8-bit integer
    - Temperature: 8-bit integer
    - Checksum = (humidity int + humidity dec + temp int + temp dec) & 0xFF

    This example code is in the Public Domain (or CC0 licensed, at your option.)
--------------------------------------------------------------------------------*/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"

#include "DHT11.h"
#include "tasks_common.h"

// == global defines =============================================

static const char* TAG = "DHT11";

int DHTgpio = 4;    // default DHT11 pin = GPIO4
float humidity = 0.;
float temperature = 0.;

// == set the DHT used pin ========================================

void setDHTgpio(int gpio)
{
    DHTgpio = gpio;
}

float getHumidity() { return humidity; }
float getTemperature() { return temperature; }

// == error handler ===============================================

void errorHandler(int response)
{
    switch(response) {
        case DHT_TIMEOUT_ERROR:
            ESP_LOGE(TAG, "Sensor Timeout");
            break;

        case DHT_CHECKSUM_ERROR:
            ESP_LOGE(TAG, "Checksum error");
            break;

        case DHT_OK:
            break;

        default:
            ESP_LOGE(TAG, "Unknown error");
    }
}

// == signal timing helper ========================================

int getSignalLevel(int usTimeOut, bool state)
{
    int uSec = 0;
    while(gpio_get_level(DHTgpio) == state) {
        if(uSec > usTimeOut)
            return -1;
        ++uSec;
        esp_rom_delay_us(1);
    }
    return uSec;
}

#define MAXdhtData 5   // 40 bits = 5 bytes

int readDHT()
{
    int uSec = 0;
    uint8_t dhtData[MAXdhtData] = {0};
    uint8_t byteInx = 0;
    uint8_t bitInx = 7;

    // == Send start signal to DHT11 ==
    gpio_set_direction(DHTgpio, GPIO_MODE_OUTPUT);

    gpio_set_level(DHTgpio, 0);
    esp_rom_delay_us(20000);  // at least 18 ms for DHT11

    gpio_set_level(DHTgpio, 1);
    esp_rom_delay_us(25);

    gpio_set_direction(DHTgpio, GPIO_MODE_INPUT);

    // == DHT response ==
    uSec = getSignalLevel(85, 0);
    if(uSec < 0) return DHT_TIMEOUT_ERROR;

    uSec = getSignalLevel(85, 1);
    if(uSec < 0) return DHT_TIMEOUT_ERROR;

    // == Read 40 bits ==
    for(int k = 0; k < 40; k++) {
        uSec = getSignalLevel(56, 0);
        if(uSec < 0) return DHT_TIMEOUT_ERROR;

        uSec = getSignalLevel(75, 1);
        if(uSec < 0) return DHT_TIMEOUT_ERROR;

        if(uSec > 40)
            dhtData[byteInx] |= (1 << bitInx);

        if(bitInx == 0) { bitInx = 7; ++byteInx; }
        else bitInx--;
    }

    // == Extract humidity and temperature ==
    humidity = dhtData[0];      // integer part only for DHT11
    temperature = dhtData[2];   // integer part only for DHT11

    // == Checksum ==
    if(dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF))
        return DHT_OK;
    else
        return DHT_CHECKSUM_ERROR;
}

// == DHT11 FreeRTOS Task ========================================

static void DHT11_task(void *pvParameter)
{
    setDHTgpio(DHT_GPIO);

    for(;;) {
        int ret = readDHT();
        errorHandler(ret);

        // Wait at least 2 sec between reads
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void DHT11_task_start(void)
{
    xTaskCreatePinnedToCore(&DHT11_task, "DHT11_task",
        DHT11_TASK_STACK_SIZE, NULL, DHT11_TASK_PRIORITY,
        NULL, DHT11_TASK_CORE_ID);
}



