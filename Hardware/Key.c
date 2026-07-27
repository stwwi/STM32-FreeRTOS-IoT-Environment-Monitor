#include "stm32f10x.h"                  // 包含 STM32 设备头文件
#include "Delay.h"                      // 包含延时函数头文件

// ==========================================
// 函数功能：按键初始化
// ==========================================
void Key_Init(void)
{
	// 1. 开启 GPIOB 的时钟，给 GPIOB 模块通电
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	// 2. 配置引脚参数
	GPIO_InitTypeDef GPIO_InitSructure;
	GPIO_InitSructure.GPIO_Mode = GPIO_Mode_IPU;             // 模式：上拉输入（默认高电平，按下低电平）
	
	// 用按位或 (|) 把 Pin_1 和 Pin_13 连起来，同时初始化这两个引脚
	GPIO_InitSructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_13; 
	
	// 对于输入模式来说，Speed 这个参数其实是不起作用的，但写上也没有任何影响
	GPIO_InitSructure.GPIO_Speed = GPIO_Speed_50MHz;         
	
	// 3. 将配置写入 GPIOB 寄存器，生效
	GPIO_Init(GPIOB, &GPIO_InitSructure);
}

// ==========================================
// 函数功能：获取按键的键码
// 返回值：  0: 没按下任何按键 / 1: 按下了PB1 / 2: 按下了PB13
// ==========================================
uint8_t Key_GetNum(void)
{
	uint8_t KeyNum = 0;  // 默认返回 0（代表没有按键按下）
	
	// ---------- 检测 1 号按键 (PB1) ----------
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)      // 如果读到 PB1 为 0（按键按下）
	{
		Delay_ms(20);                                       // 按下消抖：延时 20ms 跳过物理杂波
		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0); // 阻塞等待：死等按键松开
		Delay_ms(20);                                       // 松手消抖：延时 20ms 跳过物理杂波
		KeyNum = 1;                                         // 记录键码为 1
	}
	
	// ---------- 检测 2 号按键 (PB13) ----------
	if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0)     // 如果读到 PB13 为 0（按键按下）
	{
		Delay_ms(20);                                       // 按下消抖
		while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_13) == 0);// 阻塞等待：死等按键松开
		Delay_ms(20);                                       // 松手消抖
		KeyNum = 2;                                         // 记录键码为 2
	}

	// 将最终得到的键码返回给主函数 main
	return KeyNum;
}
