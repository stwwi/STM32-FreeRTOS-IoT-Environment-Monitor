# 基于 STM32 和 FreeRTOS 的物联网环境监测报警系统

## 项目概述

一套完整的嵌入式物联网环境监测方案。基于 STM32F103C8T6 微控制器，采用 FreeRTOS 实时操作系统实现多任务并发调度，集成温湿度采集、光照检测、本地显示、阈值报警等功能，并通过 ESP8266 WiFi 模块将环境数据上报至远程服务器，实现远程监测。

## 系统架构

```
┌─────────────────────────────────────────────────────────┐
│                    FreeRTOS 调度内核                       │
├──────────────┬──────────────────┬────────────────────────┤
│ Sensor_Task  │  Display_Task    │    ESP8266_Task         │
│ (优先级 2)   │  (优先级 1)       │    (优先级 1)           │
│              │                  │                        │
│ · DHT11 采集 │  · OLED 刷新显示  │  · WiFi 连接管理        │
│ · ADC 光照   │  · 串口日志输出   │  · HTTP 数据上报        │
│ · 报警判断   │                  │  · 断线自动重连          │
│ · 蜂鸣器控制 │                  │                        │
└──────┬───────┴────────▲─────────┴───────────▲────────────┘
       │    xSensorQueue │      xCloudQueue   │
       └────────────────-┘────────────────────-┘
              队列通信（生产者-消费者模型）
```

## 技术亮点

**1. FreeRTOS 实时多任务架构**
- 采用生产者-消费者模型，Sensor_Task 以固定 2s 周期采集数据，通过 `xQueueOverwrite` 保证消费端始终获取最新数据
- 使用 `vTaskDelayUntil` 实现精确周期调度，消除任务执行时间带来的累积误差
- Display_Task 阻塞等待队列数据，避免轮询浪费 CPU 资源，同时设置 3s 超时检测传感器任务异常

**2. ESP8266 网络通信设计**
- 基于 USART3 中断收发，通过 AT 指令集驱动 ESP8266
- 实现完整的异常处理机制：超时检测、断线自动重连、心跳保活
- HTTP GET 方式上报数据至远程服务器，支持连接状态监控

**3. 多传感器融合与报警策略**
- DHT11 温湿度采集 + 光敏电阻模拟/数字双通道检测
- ADC 读取光照模拟量（12bit，0~4095），DO 输出光暗阈值判断
- 温湿度超限自动触发蜂鸣器报警，报警状态同步上报云端

**4. 软件工程实践**
- 模块化分层设计：HAL 硬件抽象层、驱动层、应用层解耦
- 栈溢出检测钩子 + 内存分配失败钩子，增强系统健壮性
- 条件编译宏控制 ESP8266 模块启用/禁用，便于分模块调试

## 硬件方案

| 模块 | 型号/规格 | 通信方式 | MCU 引脚 |
|------|-----------|----------|----------|
| 主控 | STM32F103C8T6 (Cortex-M3, 72MHz) | — | — |
| 温湿度传感器 | DHT11 | 单总线 | PB0 |
| OLED 显示屏 | 0.96寸 128×64 | 软件 I2C | PB8(SCL) / PB9(SDA) |
| WiFi 模块 | ESP8266 | USART3 (115200bps) | PB10(TX) / PB11(RX) |
| 光敏传感器 | 光敏电阻模块 | GPIO + ADC | PA0(DO) / PA4(AO) |
| 蜂鸣器 | 有源蜂鸣器 | GPIO | PB12 |
| 串口调试 | USB-TTL | USART1 (115200bps) | PA9(TX) / PA10(RX) |

## 软件架构

```
项目目录
├── FreeRTOS/            # FreeRTOS 内核源码（V10.x）
├── FreeRTOS_CORE/       # FreeRTOS 核心组件
├── Hadwar/              # 硬件驱动层
│   ├── DHT11.c/h       #   温湿度传感器驱动
│   ├── OLED.c/h        #   OLED 显示驱动（软件I2C）
│   ├── ESP8266.c/h     #   WiFi 模块驱动（AT指令封装）
│   ├── AD.c/h          #   ADC 采集驱动
│   ├── BEEP.c/h        #   蜂鸣器控制
│   ├── LIGHT.c/h       #   光敏传感器驱动
│   └── Serial.c/h      #   串口通信（printf重定向）
├── Library/             # STM32 标准外设库（SPL）
├── Start/               # 启动文件 & CMSIS
├── System/              # 系统级功能（DWT精确延时）
├── User/                # 应用层
│   ├── main.c          #   主程序（任务创建与调度）
│   ├── FreeRTOSConfig.h#   FreeRTOS 内核配置
│   └── WifiConfig.h    #   WiFi & 服务器配置
└── Peojiect.uvprojx    # Keil MDK 工程文件
```

## 开发环境

- IDE：Keil MDK-ARM V5
- 编译器：ARMCC V5
- 调试器：ST-Link V2
- RTOS：FreeRTOS V10.x（抢占式调度，heap_4 内存管理）
- 固件库：STM32 标准外设库（SPL）

## 运行效果

串口输出示例：
```text
FreeRTOS Starting...
Task create: Sensor=1 Display=1 ESP8266=1
ESP8266 WiFi test start
ESP8266: joining WiFi SSID=xiaohei
ESP8266: WiFi joined
Temp:26C Humi:45% Light:Bright AD:2048 Alarm:0
ESP8266 alive
ESP8266 upload OK: /update?temp=26&humi=45&light=2048&alarm=0
Temp:31C Humi:72% Light:Dark AD:3856 Alarm:1
```

## 可扩展方向

- 接入 MQTT 协议对接主流物联网云平台（OneNET / 阿里云IoT）
- 增加更多传感器节点，构建多节点无线传感网络
- 开发上位机或微信小程序实现远程阈值配置与历史数据可视化
- 引入低功耗模式（Tickless Idle），适配电池供电场景
