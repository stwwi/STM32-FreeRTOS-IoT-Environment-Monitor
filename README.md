# 基于 STM32F103C8T6 和 ESP8266 的物联网环境监测报警系统

## 项目简介

本项目基于 STM32F103C8T6 单片机设计环境监测报警系统，当前已实现温湿度采集、光照检测、OLED 本地显示、蜂鸣器报警和串口调试功能。后续将接入 ESP8266 WiFi 模块，把环境数据上传到云平台，实现物联网远程监测。

## 当前已实现功能

1. DHT11 采集温度和湿度。
2. OLED 实时显示温度、湿度、光照状态和光照 ADC 数值。
3. 光敏模块 DO 判断 Bright / Dark。
4. 光敏模块 AO 通过 ADC 读取 0~4095 模拟量。
5. 蜂鸣器在温湿度超限时报警。
6. USART1 串口输出调试信息，方便观察程序运行状态。

## 后续计划功能

1. 接入 ESP8266 WiFi 模块。
2. 通过串口 AT 指令配置 ESP8266 入网。
3. 将温度、湿度、光照状态、ADC 数值和报警状态上传到云平台。
4. 在云平台或手机端查看环境数据。
5. 后续可扩展远程设置报警阈值。

## 技术栈

- STM32F103C8T6
- C 语言
- Keil MDK / uVision
- STM32 标准外设库 SPL
- GPIO
- ADC
- USART 串口
- 软件 I2C
- DHT11
- OLED
- 光敏传感器
- 蜂鸣器
- USB 转 TTL 串口调试
- ESP8266 WiFi 模块（计划接入）

## 当前接线

| 模块 | 引脚 | STM32 引脚 |
|---|---|---|
| OLED | SCL | PB8 |
| OLED | SDA | PB9 |
| DHT11 | DATA | PB0 |
| 蜂鸣器 | SIG | PB12 |
| 光敏模块 | DO | PA0 |
| 光敏模块 | AO | PA4 / ADC_Channel_4 |
| 串口调试 | TX | PA9 |
| 串口调试 | RX | PA10 |

## 串口调试输出示例

```text
Serial OK
Temp:25C Humi:42% Light:Bright AD:1234 Alarm:0
```

## 项目定位

本项目可作为 STM32 入门阶段的综合实践项目，用于学习 GPIO、ADC、USART、传感器驱动、OLED 显示、报警逻辑和后续 ESP8266 物联网通信。
