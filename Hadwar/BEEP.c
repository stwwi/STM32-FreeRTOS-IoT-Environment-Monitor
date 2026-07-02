#include "BEEP.h"

/*
 * 当前蜂鸣器接线来自你能单独跑通的“3-3 蜂鸣器”工程：
 * 蜂鸣器信号线接 PB12。
 */
#define BEEP_PORT GPIOB
#define BEEP_PIN  GPIO_Pin_12

void BEEP_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * 使用 GPIOB 之前必须先打开 GPIOB 时钟。
     * 可以理解为：先给 GPIOB 这个外设模块通电。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /*
     * PB12 配置成推挽输出。
     * 推挽输出表示 STM32 可以主动输出高电平，也可以主动输出低电平。
     */
    GPIO_InitStructure.GPIO_Pin = BEEP_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

    // 初始化完成后默认关闭蜂鸣器，避免一上电就响。
    BEEP_OFF();
}

void BEEP_ON(void)
{
    /*
     * 你的蜂鸣器模块是低电平触发：
     * PB12 输出低电平，蜂鸣器响。
     */
    GPIO_ResetBits(BEEP_PORT, BEEP_PIN);
}

void BEEP_OFF(void)
{
    /*
     * PB12 输出高电平，蜂鸣器停止。
     */
    GPIO_SetBits(BEEP_PORT, BEEP_PIN);
}
