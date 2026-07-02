#include "ESP8266.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <string.h>

#define ESP8266_RX_BUFFER_SIZE 256

static volatile uint16_t esp8266RxCount = 0;
static char esp8266RxBuffer[ESP8266_RX_BUFFER_SIZE];
static const char *esp8266LastError = "none";

static ESP8266_Status_t ESP8266_WaitFor(const char *expected, uint32_t timeoutMs)
{
    TickType_t startTick;
    TickType_t timeoutTicks;

    startTick = xTaskGetTickCount();
    timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    while ((xTaskGetTickCount() - startTick) < timeoutTicks)
    {
        if (strstr(esp8266RxBuffer, expected) != NULL)
        {
            return ESP8266_STATUS_OK;
        }

        if (strstr(esp8266RxBuffer, "ERROR") != NULL ||
            strstr(esp8266RxBuffer, "FAIL") != NULL)
        {
            return ESP8266_STATUS_ERROR;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP8266_STATUS_TIMEOUT;
}

void ESP8266_USART3_Init(uint32_t baudRate)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    usart.USART_BaudRate = baudRate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &usart);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    nvic.NVIC_IRQChannel = USART3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 6;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_Cmd(USART3, ENABLE);
}

void ESP8266_ClearBuffer(void)
{
    taskENTER_CRITICAL();
    memset(esp8266RxBuffer, 0, sizeof(esp8266RxBuffer));
    esp8266RxCount = 0;
    taskEXIT_CRITICAL();
}

void ESP8266_SendString(const char *str)
{
    while (*str != '\0')
    {
        USART_SendData(USART3, (uint8_t)*str++);
        while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    }
}

ESP8266_Status_t ESP8266_SendCommand(const char *cmd, const char *expected, uint32_t timeoutMs)
{
    TickType_t startTick;
    TickType_t timeoutTicks;

    ESP8266_ClearBuffer();
    ESP8266_SendString(cmd);

    startTick = xTaskGetTickCount();
    timeoutTicks = pdMS_TO_TICKS(timeoutMs);

    while ((xTaskGetTickCount() - startTick) < timeoutTicks)
    {
        if (strstr(esp8266RxBuffer, expected) != NULL)
        {
            return ESP8266_STATUS_OK;
        }

        if (strstr(esp8266RxBuffer, "ERROR") != NULL ||
            strstr(esp8266RxBuffer, "FAIL") != NULL)
        {
            return ESP8266_STATUS_ERROR;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    return ESP8266_STATUS_TIMEOUT;
}

ESP8266_Status_t ESP8266_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];
    uint8_t retry;

    for (retry = 0; retry < 5; retry++)
    {
        if (ESP8266_SendCommand("AT\r\n", "OK", 1000) == ESP8266_STATUS_OK)
        {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if (retry >= 5)
    {
        return ESP8266_STATUS_TIMEOUT;
    }

    (void)ESP8266_SendCommand("ATE0\r\n", "OK", 1000);

    if (ESP8266_SendCommand("AT+CWMODE=1\r\n", "OK", 2000) != ESP8266_STATUS_OK)
    {
        return ESP8266_STATUS_ERROR;
    }

    (void)ESP8266_SendCommand("AT+CWDHCP=1,1\r\n", "OK", 2000);
    (void)ESP8266_SendCommand("AT+CIPMUX=0\r\n", "OK", 2000);

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    if (ESP8266_SendCommand(cmd, "GOT IP", 20000) == ESP8266_STATUS_OK)
    {
        return ESP8266_STATUS_OK;
    }

    if (strstr(esp8266RxBuffer, "OK") != NULL)
    {
        return ESP8266_STATUS_OK;
    }

    return ESP8266_STATUS_TIMEOUT;
}

ESP8266_Status_t ESP8266_HTTPGet(const char *host, uint16_t port, const char *path)
{
    char cmd[80];
    char request[220];
    int requestLen;
    uint8_t retry;
    ESP8266_Status_t startStatus;

    startStatus = ESP8266_STATUS_ERROR;
    for (retry = 0; retry < 2; retry++)
    {
        vTaskDelay(pdMS_TO_TICKS(300));
        snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", host, port);
        startStatus = ESP8266_SendCommand(cmd, "CONNECT", 8000);
        if (startStatus == ESP8266_STATUS_OK ||
            strstr(esp8266RxBuffer, "ALREADY CONNECTED") != NULL ||
            strstr(esp8266RxBuffer, "ALREADY CONNECT") != NULL)
        {
            break;
        }

        (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (startStatus != ESP8266_STATUS_OK &&
        strstr(esp8266RxBuffer, "ALREADY CONNECTED") == NULL &&
        strstr(esp8266RxBuffer, "ALREADY CONNECT") == NULL)
    {
        esp8266LastError = "CIPSTART";
        return ESP8266_STATUS_ERROR;
    }

    requestLen = snprintf(request,
                          sizeof(request),
                          "GET %s HTTP/1.1\r\n"
                          "Host: %s:%u\r\n"
                          "Connection: close\r\n"
                          "\r\n",
                          path,
                          host,
                          port);
    if (requestLen <= 0 || requestLen >= (int)sizeof(request))
    {
        (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000);
        esp8266LastError = "REQUEST_TOO_LONG";
        return ESP8266_STATUS_ERROR;
    }

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", requestLen);
    if (ESP8266_SendCommand(cmd, ">", 3000) != ESP8266_STATUS_OK)
    {
        (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000);
        esp8266LastError = "CIPSEND";
        return ESP8266_STATUS_ERROR;
    }

    ESP8266_ClearBuffer();
    ESP8266_SendString(request);

    if (ESP8266_WaitFor("200 OK", 5000) == ESP8266_STATUS_OK ||
        strstr(esp8266RxBuffer, "OK") != NULL ||
        strstr(esp8266RxBuffer, "SEND OK") != NULL)
    {
        (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000);
        esp8266LastError = "none";
        return ESP8266_STATUS_OK;
    }

    (void)ESP8266_SendCommand("AT+CIPCLOSE\r\n", "OK", 1000);
    esp8266LastError = "HTTP_RESPONSE";
    return ESP8266_STATUS_TIMEOUT;
}

const char *ESP8266_GetLastError(void)
{
    return esp8266LastError;
}

void ESP8266_USART3_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        data = (uint8_t)USART_ReceiveData(USART3);

        if (esp8266RxCount < (ESP8266_RX_BUFFER_SIZE - 1))
        {
            esp8266RxBuffer[esp8266RxCount++] = (char)data;
            esp8266RxBuffer[esp8266RxCount] = '\0';
        }

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

const char *ESP8266_GetRxBuffer(void)
{
    return esp8266RxBuffer;
}
