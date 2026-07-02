# FreeRTOS 移植方案

## 项目现状
- MCU: STM32F103C8（64KB Flash, 20KB RAM）
- 工具链: Keil MDK + StdPeriph Library
- 当前架构: 裸机 while(1) 超级循环
- Delay: 直接操作 SysTick 寄存器实现 us/ms 延时

## 改造目标
将裸机项目改造为 FreeRTOS 双任务架构：
- **任务1 (Sensor_Task)**: 读取 DHT11 + 光敏传感器，判断报警，控制蜂鸣器
- **任务2 (Display_Task)**: OLED 显示 + 串口输出

两个任务之间用 **队列 (Queue)** 传递传感器数据。

---

## 需要的 FreeRTOS 文件（正点原子/野火移植包结构）

在项目根目录新建 `FreeRTOS/` 文件夹，从移植包中拷贝以下文件：

```
FreeRTOS/
├── include/            ← FreeRTOS 全部头文件（约15个 .h）
│   ├── FreeRTOS.h
│   ├── task.h
│   ├── queue.h
│   ├── semphr.h
│   ├── list.h
│   ├── portable.h
│   └── ... (其他头文件)
├── portable/
│   ├── RVDS/ARM_CM3/   ← Keil 用的 Cortex-M3 端口
│   │   ├── port.c
│   │   └── portmacro.h
│   └── MemMang/
│       └── heap_4.c    ← 内存管理方案（heap_4 最通用）
├── tasks.c
├── queue.c
├── list.c
└── timers.c            ← 如果不用软件定时器可以不加
```

---

## 代码改动清单

### 1. 新增文件: `User/FreeRTOSConfig.h`
FreeRTOS 配置文件，定义堆大小、优先级数量、Tick 频率等。
- 堆大小: 4096 字节（20KB RAM 中留足够空间）
- Tick 频率: 1000Hz（1ms 一次）
- 最大优先级: 5
- 最小任务栈: 128 字（512 字节）

### 2. 修改: `User/stm32f10x_it.c`
**删除** 以下三个空函数（FreeRTOS 的 port.c 会提供它们）：
- `SVC_Handler()`
- `PendSV_Handler()`
- `SysTick_Handler()`

### 3. 修改: `System/Delay.c`
FreeRTOS 接管 SysTick 后，原来直接操作 SysTick 的 Delay_us 会冲突。改为：
- `Delay_us()`: 用 DWT 周期计数器实现（Cortex-M3 自带，不占外设）
- `Delay_ms()`: 在任务上下文中用 `vTaskDelay()`，在初始化阶段用循环调 Delay_us

### 4. 修改: `User/main.c`（重写）
- 创建全局队列句柄
- 定义传感器数据结构体
- 创建两个任务
- 启动调度器 `vTaskStartScheduler()`

### 5. 硬件驱动文件（OLED/DHT11/BEEP/LIGHT/AD/Serial）
**不需要改动**，保持原样。唯一注意：
- DHT11 读取时序要求精确微秒延时，在读取期间用 `taskENTER_CRITICAL()` 关中断保护。

---

## 任务设计

```
┌─────────────────┐         Queue          ┌─────────────────┐
│  Sensor_Task    │ ───── SensorData ─────▶ │  Display_Task   │
│  优先级: 2      │                         │  优先级: 1      │
│                 │                         │                 │
│  - DHT11读取    │                         │  - OLED显示     │
│  - 光敏读取    │                         │  - 串口输出     │
│  - 报警判断    │                         │                 │
│  - 蜂鸣器控制  │                         │                 │
│  - 每2秒一次   │                         │  - 收到数据就刷新│
└─────────────────┘                         └─────────────────┘
```

Sensor_Task 优先级高于 Display_Task，保证传感器读取不被打断。

---

## RAM 预估

| 项目 | 大小 |
|------|------|
| FreeRTOS 堆 (heap_4) | 4096 B |
| Sensor_Task 栈 | 128 × 4 = 512 B |
| Display_Task 栈 | 256 × 4 = 1024 B |
| 全局变量 + 队列 | ~500 B |
| **合计** | **~6 KB / 20 KB** |

RAM 充裕，没有问题。

---

## 实施步骤

1. 从正点原子/野火移植包中拷贝 FreeRTOS 源码到项目 `FreeRTOS/` 目录
2. 在 Keil 工程中添加 FreeRTOS 源文件和 include 路径
3. 我生成 `FreeRTOSConfig.h`
4. 我修改 `stm32f10x_it.c`（删除三个冲突的 Handler）
5. 我重写 `Delay.c`（改用 DWT）
6. 我修改 `DHT11.c`（加临界区保护）
7. 我重写 `main.c`（任务创建 + 队列 + 调度器启动）
8. Keil 中编译验证

步骤 1-2 需要你在 Keil 中手动操作（添加文件到工程组、设置 include 路径）。
步骤 3-7 我直接帮你生成代码。
