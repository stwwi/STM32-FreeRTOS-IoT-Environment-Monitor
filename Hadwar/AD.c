#include "AD.h"

void AD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    /*
     * ADC1 和 GPIOA 都在 APB2 总线上。
     * 读取 PA4 的模拟电压前，必须先打开它们的时钟。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /*
     * ADC 时钟不能太快。
     * 系统时钟 72MHz，6 分频后 ADC 时钟是 12MHz，适合 STM32F103。
     */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    /*
     * PA4 配置成模拟输入。
     * 模拟输入和普通数字输入不同，它要把外部连续变化的电压送进 ADC。
     */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * 只转换一个通道：ADC1 通道 4，也就是 PA4。
     */
    ADC_RegularChannelConfig(ADC1, ADC_Channel_4, 1, ADC_SampleTime_55Cycles5);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_Cmd(ADC1, ENABLE);

    /*
     * ADC 校准是固定流程。
     * 校准后，转换结果会更稳定。
     */
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);
}

uint16_t AD_GetValue(void)
{
    /*
     * 软件触发一次转换。
     * ADC 会把 PA4 上的模拟电压转换成 0~4095 的数字值。
     */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);

    return ADC_GetConversionValue(ADC1);
}
