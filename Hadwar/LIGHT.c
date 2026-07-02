#include "LIGHT.h"

#define LIGHT_PORT GPIOA
#define LIGHT_PIN  GPIO_Pin_0

void LIGHT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * 使用 GPIOA 之前先打开 GPIOA 时钟。
     * 光敏模块 DO 接 PA0，所以这里开启 GPIOA。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /*
     * PA0 设置成上拉输入。
     * 输入模式下，STM32 不主动输出电平，只读取模块 DO 当前是高还是低。
     */
    GPIO_InitStructure.GPIO_Pin = LIGHT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LIGHT_PORT, &GPIO_InitStructure);
}

uint8_t LIGHT_GetDO(void)
{
    /*
     * 返回 PA0 当前电平：
     * 0 = 低电平
     * 1 = 高电平
     *
     * 先观察遮光/照光时这个数怎么变化，再决定 0 或 1 代表亮还是暗。
     */
    return GPIO_ReadInputDataBit(LIGHT_PORT, LIGHT_PIN);
}
