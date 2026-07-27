#ifndef __AD_H
#define __AD_H

#include "stm32f10x.h"

/*
 * ADC 模数转换模块。
 * 当前用于读取光敏模块 AO 模拟输出。
 *
 * 接线：
 *   光敏 AO -> PA4
 *
 * PA4 对应 ADC1 的通道 4，所以代码里使用 ADC_Channel_4。
 */
void AD_Init(void);
uint16_t AD_GetValue(void);

#endif
