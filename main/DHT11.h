/* 
    DHT11 temperature & humidity sensor driver
    Built by BenBr
*/

#ifndef DHT11_H_  
#define DHT11_H_

#define DHT_OK              0
#define DHT_CHECKSUM_ERROR -1
#define DHT_TIMEOUT_ERROR  -2

#include "stdbool.h"
// Default GPIO (you can override in code with setDHTgpio())
#define DHT_GPIO            4

/**
 * Starts DHT11 sensor task
 */
void DHT11_task_start(void);

// == function prototypes =======================================

void    setDHTgpio(int gpio);
void    errorHandler(int response);
int     readDHT();
float   getHumidity();
float   getTemperature();
int     getSignalLevel(int usTimeOut, bool state);

#endif /* DHT11_H_ */
