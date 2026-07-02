#include "stm32f10x.h"

// ==========================================
// 函数功能：LED 初始化
// ==========================================
void LED_Init(void)
{
	// 1. 开启 GPIOA 的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	// 2. 配置引脚模式
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;        // 推挽输出模式
	// 用按位或 (|) 把 Pin_1 和 Pin_2 连起来，可以同时初始化这两个引脚，非常高效！
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;       // 输出速度 50MHz
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	// 3. 设置默认状态：初始化后默认输出高电平（让两盏 LED 默认处于熄灭状态）
	GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);
}

// ==========================================
// 函数功能：控制 LED1 (PA1) 亮灭
// ==========================================
void LED1_ON(void)
{
	// 拉低 PA1 电平，点亮 LED1
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}

void LED1_OFF(void)
{
	// 拉高 PA1 电平，熄灭 LED1
	GPIO_SetBits(GPIOA, GPIO_Pin_1);
}

// ==========================================
// 函数功能：控制 LED2 (PA2) 亮灭
// ==========================================
void LED2_ON(void)
{
	// 拉低 PA2 电平，点亮 LED2
	GPIO_ResetBits(GPIOA, GPIO_Pin_2);
}

void LED2_OFF(void)
{
	// 拉高 PA2 电平，熄灭 LED2
	GPIO_SetBits(GPIOA, GPIO_Pin_2);
}

// ==========================================
// 函数功能：控制 LED 状态翻转 (Toggle)
// ==========================================
void LED1_Turn(void)
{
	// 读取 GPIOA 端口第 1 号引脚 (PA1) 当前的输出状态
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)
	{
		// 如果当前输出的是低电平（灯亮着），就把它拉高（熄灭）
		GPIO_SetBits(GPIOA, GPIO_Pin_1);
	}
	else
	{
		// 否则当前输出的是高电平（灯灭着），就把它拉低（点亮）
		GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	}
}

void LED2_Turn(void)
{
	// 读取 GPIOA 端口第 2 号引脚 (PA2) 当前的输出状态 （注：这里的注释帮你修正啦！）
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0)
	{
		// 如果当前输出的是低电平（灯亮着），就把它拉高（熄灭）
		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	}
	else
	{
		// 否则当前输出的是高电平（灯灭着），就把它拉低（点亮）
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
	}
}