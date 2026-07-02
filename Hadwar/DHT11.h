#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

/*
 * DHT11 温湿度传感器驱动头文件。
 *
 * 当前接线：
 * DHT11 DATA -> PB0
 * DHT11 VCC  -> 3.3V
 * DHT11 GND  -> GND
 */

void DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi);

#endif
