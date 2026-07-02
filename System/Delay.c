#include "stm32f10x.h"
#include "Delay.h"

/*
 * 用裸寄存器地址直接访问 DWT，兼容所有版本的 CMSIS。
 * Cortex-M3 DWT 寄存器地址是固定的，不受 CMSIS 版本影响。
 */
#define DWT_CTRL_REG   (*(volatile uint32_t *)0xE0001000)
#define DWT_CYCCNT_REG (*(volatile uint32_t *)0xE0001004)
#define DEM_CR_REG     (*(volatile uint32_t *)0xE000EDFC)

void Delay_DWT_Init(void)
{
    DEM_CR_REG  |= (1UL << 24);  /* 开启 TRCENA，允许 DWT */
    DWT_CYCCNT_REG = 0;           /* 清零计数器 */
    DWT_CTRL_REG   |= (1UL << 0); /* 启用 CYCCNT */
}

void Delay_us(uint32_t xus)
{
    uint32_t start  = DWT_CYCCNT_REG;
    uint32_t target = xus * 72u;   /* 72MHz -> 1us = 72 cycles */

    while ((DWT_CYCCNT_REG - start) < target);
}

void Delay_ms(uint32_t xms)
{
    while (xms--)
    {
        Delay_us(1000);
    }
}

void Delay_s(uint32_t xs)
{
    while (xs--)
    {
        Delay_ms(1000);
    }
}
