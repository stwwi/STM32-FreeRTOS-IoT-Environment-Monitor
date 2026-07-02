#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*
 * FreeRTOS 配置文件 — 适配 STM32F103C8 (Cortex-M3, 72MHz, 20KB RAM)
 *
 * 说明：
 * 这个文件告诉 FreeRTOS 内核你的硬件有什么、你想用哪些功能。
 * 如果以后想增加功能（比如软件定时器、互斥量统计），改这里就行。
 */

/* ========== 基本内核配置 ========== */

/* 1 = 抢占式调度（高优先级任务能打断低优先级任务）。这是最常用的模式。 */
#define configUSE_PREEMPTION                    1

/* CPU 主频，单位 Hz。STM32F103C8 跑 72MHz。 */
#define configCPU_CLOCK_HZ                      ((unsigned long)72000000)

/* Tick 中断频率。1000 表示 1ms 一次 Tick，vTaskDelay(1000) = 1秒。 */
#define configTICK_RATE_HZ                      ((TickType_t)1000)

/* 最大优先级数量。数字越大优先级越高。5 级对小项目足够了。 */
#define configMAX_PRIORITIES                    5

/* 空闲任务栈大小（单位：字=4字节）。128字 = 512字节，够用。 */
#define configMINIMAL_STACK_SIZE                ((unsigned short)128)

/* FreeRTOS heap used by tasks and queues. 8KB is enough for sensor, display,
 * and ESP8266 test tasks on STM32F103C8's 20KB SRAM. */
#define configTOTAL_HEAP_SIZE                   ((size_t)(8192))

/* 任务名字最大长度，调试时能在 Keil 里看到任务名。 */
#define configMAX_TASK_NAME_LEN                 16

/* 使用 16 位 Tick 计数器（0）还是 32 位（1）。32 位不容易溢出。 */
#define configUSE_16_BIT_TICKS                  0

/* 同优先级任务之间是否轮转调度。1 = 是。 */
#define configIDLE_SHOULD_YIELD                 1

/* ========== 功能开关 ========== */

/* 使用互斥量。暂时没用到，但开着不占资源，以后扩展方便。 */
#define configUSE_MUTEXES                       1

/* 使用计数信号量 */
#define configUSE_COUNTING_SEMAPHORES           0

/* 使用递归互斥量 */
#define configUSE_RECURSIVE_MUTEXES             0

/* 队列相关（本项目用队列传数据） */
#define configQUEUE_REGISTRY_SIZE               8

/* 软件定时器（暂不使用） */
#define configUSE_TIMERS                        0
#define configTIMER_TASK_PRIORITY               2
#define configTIMER_QUEUE_LENGTH                10
#define configTIMER_TASK_STACK_DEPTH            configMINIMAL_STACK_SIZE

/* ========== 钩子函数 ========== */

/* 空闲任务钩子（0=不用） */
#define configUSE_IDLE_HOOK                     0

/* Tick 钩子（0=不用） */
#define configUSE_TICK_HOOK                     0

/* 栈溢出检测。1 = 检测方式一（简单快速），开发阶段建议开着。 */
#define configCHECK_FOR_STACK_OVERFLOW          1

/* malloc 失败钩子。开发阶段开着，内存不够时能及时发现。 */
#define configUSE_MALLOC_FAILED_HOOK            1

/* ========== 运行时统计（暂不开启） ========== */
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_TRACE_FACILITY                0
#define configUSE_STATS_FORMATTING_FUNCTIONS    0

/* ========== 协程（不用） ========== */
#define configUSE_CO_ROUTINES                   0
#define configMAX_CO_ROUTINE_PRIORITIES         2

/* ========== 可选 API 函数 ========== */
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1

/* ========== Cortex-M3 中断优先级配置 ========== */

/*
 * STM32F103 的 NVIC 用了 4 位优先级（16 个等级，0~15）。
 * 数字越小，优先级越高。
 *
 * FreeRTOS 管理的中断优先级不能高于 configMAX_SYSCALL_INTERRUPT_PRIORITY，
 * 否则在中断里调用 FreeRTOS API 会出错。
 */

/* Cortex-M3 用高 4 位表示优先级，所以要左移 4 位 */
#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS 4
#endif

/* 最低优先级（数值最大） */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15

/* FreeRTOS 可管理的最高中断优先级。
 * 设为 5 表示优先级 0~4 的中断不受 FreeRTOS 管理（不能在里面调 OS API）。
 * 优先级 5~15 的中断可以安全调用 FromISR 系列函数。 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5

/* 内核用的 PendSV 和 SysTick 优先级（最低） */
#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* ========== FreeRTOS 中断服务函数映射 ========== */

/*
 * 把 FreeRTOS 内部用的函数名映射到 STM32 的标准中断向量名。
 * 这样 port.c 里定义的 xPortPendSVHandler 等就能被启动文件正确调用。
 */
#define vPortSVCHandler         SVC_Handler
#define xPortPendSVHandler      PendSV_Handler
#define xPortSysTickHandler     SysTick_Handler

#endif /* FREERTOS_CONFIG_H */
