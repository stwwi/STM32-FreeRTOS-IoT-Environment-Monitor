#ifndef __LIGHT_H
#define __LIGHT_H

#include "stm32f10x.h"

/*
 * 光敏传感器 DO 数字输出驱动。
 *
 * 当前接线：
 * 光敏模块 DO  -> PA0
 * 光敏模块 VCC -> 3.3V
 * 光敏模块 GND -> GND
 *
 * 这里先读 DO 的 0/1，不直接判断亮暗。
 * 因为不同光敏模块的 DO 触发方向可能相反，需要先观察实际电平。
 */

void LIGHT_Init(void);
uint8_t LIGHT_GetDO(void);

#endif
