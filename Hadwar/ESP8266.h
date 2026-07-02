#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f10x.h"
#include <stdint.h>

typedef enum
{
    ESP8266_STATUS_OK = 0,
    ESP8266_STATUS_TIMEOUT,
    ESP8266_STATUS_ERROR
} ESP8266_Status_t;

void ESP8266_USART3_Init(uint32_t baudRate);
void ESP8266_ClearBuffer(void);
void ESP8266_SendString(const char *str);
ESP8266_Status_t ESP8266_SendCommand(const char *cmd, const char *expected, uint32_t timeoutMs);
ESP8266_Status_t ESP8266_ConnectWiFi(const char *ssid, const char *password);
ESP8266_Status_t ESP8266_HTTPGet(const char *host, uint16_t port, const char *path);
const char *ESP8266_GetLastError(void);
void ESP8266_USART3_IRQHandler(void);
const char *ESP8266_GetRxBuffer(void);

#endif
