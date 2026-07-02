#include "Serial.h"
#include <stdarg.h>

void Serial_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /*
     * USART1 和 GPIOA 都挂在 APB2 总线上。
     * 使用 PA9/PA10 之前，必须先打开 GPIOA 和 USART1 的时钟。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /*
     * PA9 是 USART1_TX。
     * 复用推挽输出：这个引脚不再当普通 GPIO，而是交给 USART1 发送数据。
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * PA10 是 USART1_RX。
     * 本项目现在主要发送数据，但把 RX 也初始化好，后面扩展电脑发命令会更方便。
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void Serial_SendByte(uint8_t byte)
{
    USART_SendData(USART1, byte);

    /*
     * TXE 表示发送数据寄存器空了。
     * 等到它为空，才能安全发送下一个字节。
     */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void Serial_SendString(char *string)
{
    uint8_t i;

    for (i = 0; string[i] != '\0'; i++)
    {
        Serial_SendByte(string[i]);
    }
}

int fputc(int ch, FILE *f)
{
    Serial_SendByte((uint8_t)ch);
    return ch;
}

void Serial_Printf(char *format, ...)
{
    char buffer[120];
    va_list args;

    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    Serial_SendString(buffer);
}
