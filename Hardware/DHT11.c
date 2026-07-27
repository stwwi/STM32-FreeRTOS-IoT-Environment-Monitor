#include "stm32f10x.h"
#include "Delay.h"
#include "DHT11.h"
#include "FreeRTOS.h"
#include "task.h"

/*
 * DHT11 只用一根 DATA 数据线和 STM32 通信。
 * 当前项目把 DATA 接到 PB0，所以后面所有操作都围绕 GPIOB 的 0 号引脚。
 */
#define DHT11_PORT GPIOB
#define DHT11_PIN  GPIO_Pin_0

/*
 * 把 PB0 设置成输出模式。
 * 当 STM32 要主动控制 DATA 线时，就需要输出模式。
 */
void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

/*
 * 把 PB0 设置成输入模式。
 * 当 DHT11 要把数据发回来时，STM32 不能再控制 DATA 线，只能读取它。
 */
void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(DHT11_PORT, &GPIO_InitStructure);
}

void DHT11_Init(void)
{
    /*
     * 使用 GPIOB 之前必须先打开 GPIOB 时钟。
     * 如果不打开时钟，后面对 PB0 的配置不会生效。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 默认先让 STM32 控制 DATA 线。
    DHT11_SetOutput();

    // DHT11 的 DATA 线空闲时应该保持高电平。
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
}

/*
 * 发送开始信号。
 * DHT11 不会主动发数据，必须先由 STM32 拉低 DATA 线约 18ms 来通知它。
 */
void DHT11_Start(void)
{
    DHT11_SetOutput();

    // STM32 拉低 DATA 线 18ms，告诉 DHT11：我要读数据。
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_ms(18);

    // 拉高 DATA 线，等待一小段时间后切换成输入，准备听 DHT11 回应。
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30);

    DHT11_SetInput();
}

/*
 * 检查 DHT11 是否回应。
 *
 * 正常流程：
 * 1. STM32 发完开始信号后，DHT11 会先拉低 DATA 约 80us
 * 2. 然后 DHT11 再拉高 DATA 约 80us
 * 3. 之后才开始发送 40 位数据
 *
 * 返回值：
 * 0 = 回应正常
 * 1 = 超时失败
 */
uint8_t DHT11_CheckResponse(void)
{
    uint16_t timeout = 0;

    // 等待 DHT11 把 DATA 从高电平拉低。
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
    {
        timeout++;
        Delay_us(1);

        if (timeout > 100)
        {
            return 1;
        }
    }

    timeout = 0;

    // 等待 DHT11 的低电平回应结束。
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0)
    {
        timeout++;
        Delay_us(1);

        if (timeout > 100)
        {
            return 1;
        }
    }

    timeout = 0;

    // 等待 DHT11 的高电平回应结束。
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
    {
        timeout++;
        Delay_us(1);

        if (timeout > 100)
        {
            return 1;
        }
    }

    return 0;
}

/*
 * 读取 1 位数据。
 *
 * DHT11 每发送 1 位数据时：
 * 先拉低约 50us，然后拉高一段时间。
 *
 * 高电平短，大约 26~28us，表示 0。
 * 高电平长，大约 70us，表示 1。
 */
uint8_t DHT11_ReadBit(void)
{
    uint16_t timeout = 0;

    // 每一位开始前，DHT11 会先拉低 DATA。这里等待低电平结束。
    while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0)
    {
        timeout++;
        Delay_us(1);

        if (timeout > 100)
        {
            return 0;
        }
    }

    /*
     * 等 40us 后判断 DATA 状态：
     * 如果已经变低，说明高电平很短，是 0。
     * 如果仍然是高电平，说明高电平较长，是 1。
     */
    Delay_us(40);

    if (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
    {
        timeout = 0;

        // 如果读到的是 1，还要等这段高电平结束，准备读取下一位。
        while (GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
        {
            timeout++;
            Delay_us(1);

            if (timeout > 100)
            {
                break;
            }
        }

        return 1;
    }
    else
    {
        return 0;
    }
}

/*
 * 读取 1 个字节，也就是连续读取 8 位。
 */
uint8_t DHT11_ReadByte(void)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8; i++)
    {
        // 先左移一位，给新读到的 bit 腾位置。
        data <<= 1;

        // 把刚读到的 0 或 1 放到 data 的最低位。
        data |= DHT11_ReadBit();
    }

    return data;
}

/*
 * 读取一次温度和湿度。
 *
 * DHT11 一次会发 5 个字节：
 * 第 1 字节：湿度整数
 * 第 2 字节：湿度小数
 * 第 3 字节：温度整数
 * 第 4 字节：温度小数
 * 第 5 字节：校验和
 */
uint8_t DHT11_Read_Data(uint8_t *temp, uint8_t *humi)
{
    uint8_t humi_int;
    uint8_t humi_dec;
    uint8_t temp_int;
    uint8_t temp_dec;
    uint8_t check_sum;
    uint8_t result;

    /*
     * 进入临界区：关闭所有可屏蔽中断。
     * DHT11 的通信时序精确到微秒级别，如果中途被 FreeRTOS 调度器
     * 或其他中断打断，读到的数据就会全部错乱。
     * 所以从发送起始信号到读完 5 个字节，必须一口气完成。
     */
    taskENTER_CRITICAL();

    DHT11_Start();

    if (DHT11_CheckResponse() != 0)
    {
        taskEXIT_CRITICAL();
        return 1;
    }

    humi_int = DHT11_ReadByte();
    humi_dec = DHT11_ReadByte();
    temp_int = DHT11_ReadByte();
    temp_dec = DHT11_ReadByte();
    check_sum = DHT11_ReadByte();

    /* 通信完成，退出临界区，恢复中断和调度 */
    taskEXIT_CRITICAL();

    /*
     * 校验数据是否可靠。
     * DHT11 规定：校验和 = 前 4 个字节相加后的低 8 位。
     */
    if (check_sum == (uint8_t)(humi_int + humi_dec + temp_int + temp_dec))
    {
        *humi = humi_int;
        *temp = temp_int;
        result = 0;
    }
    else
    {
        result = 1;
    }

    return result;
}
