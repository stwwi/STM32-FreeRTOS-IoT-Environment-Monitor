#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f10x.h"

/*
 * 蜂鸣器驱动头文件。
 * 头文件只放“函数声明”，告诉 main.c 可以调用哪些函数。
 */

void BEEP_Init(void);   // 初始化蜂鸣器引脚
void BEEP_ON(void);     // 蜂鸣器响
void BEEP_OFF(void);    // 蜂鸣器停

#endif
