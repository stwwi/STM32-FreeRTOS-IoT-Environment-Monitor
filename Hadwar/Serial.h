#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>

/*
 * 串口调试模块。
 * 当前使用 USART1：
 *   PA9  -> TX，接 USB 转 TTL 的 RXD
 *   PA10 -> RX，接 USB 转 TTL 的 TXD（本项目暂时不用接收，也可以先不接）
 *   GND  -> USB 转 TTL 的 GND
 */
void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendString(char *string);
void Serial_Printf(char *format, ...);

#endif
