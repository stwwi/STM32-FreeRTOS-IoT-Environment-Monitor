#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "Delay.h"
#include "OLED.h"
#include "DHT11.h"
#include "BEEP.h"
#include "LIGHT.h"
#include "Serial.h"
#include "AD.h"
#include "ESP8266.h"
#include "WifiConfig.h"
#include <string.h>

/* 报警阈值 */
#define TEMP_MAX 30
#define HUMI_MAX 70

/* ========== 任务栈大小（单位：字=4字节） ========== */
#define SENSOR_TASK_STACK_SIZE  256
#define DISPLAY_TASK_STACK_SIZE 256
#define ESP8266_TASK_STACK_SIZE 256

/* ========== 任务优先级 ========== */
#define SENSOR_TASK_PRIORITY   2
#define DISPLAY_TASK_PRIORITY  1
#define ESP8266_TASK_PRIORITY  1

/*
 * 通过队列在两个任务之间传递的数据包。
 * Sensor_Task 填好数据后发给 Display_Task 显示。
 */
typedef struct
{
    uint8_t  temp;      /* DHT11 温度整数 */
    uint8_t  humi;      /* DHT11 湿度整数 */
    uint8_t  dht_ok;    /* 0=读取成功，1=读取失败 */
    uint8_t  light_do;  /* 光敏 DO：0=亮，1=暗 */
    uint16_t light_ad;  /* 光敏 AO ADC 值 0~4095 */
    uint8_t  alarm;     /* 0=正常，1=报警 */
} SensorData_t;

/* 队列句柄：全局，两个任务都要用 */
static QueueHandle_t xSensorQueue;
static QueueHandle_t xCloudQueue;

/* ========== 任务函数声明 ========== */
static void Sensor_Task(void *pvParameters);
static void Display_Task(void *pvParameters);
static void ESP8266_Task(void *pvParameters);

/* ========== FreeRTOS 钩子函数（FreeRTOSConfig.h 里开启了） ========== */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /*
     * 栈溢出时会进这里。
     * 开发阶段直接死循环，方便 Keil 调试器定位是哪个任务出了问题。
     */
    (void)xTask;
    (void)pcTaskName;
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    /* FreeRTOS 堆内存不足时会进这里，检查 configTOTAL_HEAP_SIZE */
    while (1);
}

/* ========== main ========== */

int main(void)
{
    BaseType_t sensorTaskResult;
    BaseType_t displayTaskResult;
    BaseType_t esp8266TaskResult;

    /* 初始化 DWT 周期计数器，供 Delay_us 使用 */
    Delay_DWT_Init();

    /* 初始化所有硬件外设 */
    OLED_Init();
    DHT11_Init();
    BEEP_Init();
    LIGHT_Init();
    Serial_Init();
    AD_Init();
#if ESP8266_ENABLE
    ESP8266_USART3_Init(115200);
#endif

    /* 启动画面 */
    OLED_ShowString(1, 1, "FreeRTOS Init");
    Serial_Printf("FreeRTOS Starting...\r\n");

    /*
     * 创建队列：深度为 1，每个元素是 SensorData_t 结构体。
     * 深度设为 1 即可，Sensor_Task 每次都覆盖最新数据。
     */
    xSensorQueue = xQueueCreate(1, sizeof(SensorData_t));
    xCloudQueue = xQueueCreate(1, sizeof(SensorData_t));

    /* 创建 Sensor_Task，优先级 2（高） */
    sensorTaskResult = xTaskCreate(Sensor_Task,
                                   "Sensor",
                                   SENSOR_TASK_STACK_SIZE,
                                   NULL,
                                   SENSOR_TASK_PRIORITY,
                                   NULL);

    /* 创建 Display_Task，优先级 1（低） */
    displayTaskResult = xTaskCreate(Display_Task,
                                    "Display",
                                    DISPLAY_TASK_STACK_SIZE,
                                    NULL,
                                    DISPLAY_TASK_PRIORITY,
                                    NULL);

#if ESP8266_ENABLE
    esp8266TaskResult = xTaskCreate(ESP8266_Task,
                                    "ESP8266",
                                    ESP8266_TASK_STACK_SIZE,
                                    NULL,
                                    ESP8266_TASK_PRIORITY,
                                    NULL);
#else
    esp8266TaskResult = pdPASS;
    Serial_Printf("ESP8266 disabled for OLED test\r\n");
#endif

    Serial_Printf("Task create: Sensor=%d Display=%d ESP8266=%d\r\n",
                  sensorTaskResult,
                  displayTaskResult,
                  esp8266TaskResult);

    if (xSensorQueue == NULL ||
        xCloudQueue == NULL ||
        sensorTaskResult != pdPASS ||
        displayTaskResult != pdPASS ||
        esp8266TaskResult != pdPASS)
    {
        Serial_Printf("Queue/task create failed, check FreeRTOS heap\r\n");
        while (1);
    }

    /* 启动 FreeRTOS 调度器，之后 main 不会再返回 */
    vTaskStartScheduler();

    /* 正常情况下不会执行到这里 */
    while (1);
}

static void ESP8266_Task(void *pvParameters)
{
    ESP8266_Status_t status;
    SensorData_t uploadData;
    char path[120];

    (void)pvParameters;

    vTaskDelay(pdMS_TO_TICKS(1500));
    Serial_Printf("ESP8266 WiFi test start\r\n");

    while (1)
    {
        Serial_Printf("ESP8266: joining WiFi SSID=%s\r\n", WIFI_SSID);

        status = ESP8266_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD);

        if (status == ESP8266_STATUS_OK)
        {
            Serial_Printf("ESP8266: WiFi joined\r\n");
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (ESP8266_SendCommand("AT+CIFSR\r\n", "OK", 3000) != ESP8266_STATUS_OK)
            {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            if (strstr(ESP8266_GetRxBuffer(), "STAIP") != NULL ||
                ESP8266_SendCommand("AT+CIFSR\r\n", "OK", 3000) == ESP8266_STATUS_OK)
            {
                Serial_Printf("ESP8266 IP info:\r\n%s\r\n", ESP8266_GetRxBuffer());
            }

            while (1)
            {
                if (ESP8266_SendCommand("AT\r\n", "OK", 1000) == ESP8266_STATUS_OK)
                {
                    Serial_Printf("ESP8266 alive\r\n");

                    if (xQueuePeek(xCloudQueue, &uploadData, 0) == pdTRUE &&
                        uploadData.dht_ok == 0)
                    {
                        snprintf(path,
                                 sizeof(path),
                                 "/update?temp=%u&humi=%u&light=%u&alarm=%u",
                                 uploadData.temp,
                                 uploadData.humi,
                                 uploadData.light_ad,
                                 uploadData.alarm);

                        if (ESP8266_HTTPGet(SERVER_IP, SERVER_PORT, path) == ESP8266_STATUS_OK)
                        {
                            Serial_Printf("ESP8266 upload OK: %s\r\n", path);
                        }
                        else
                        {
                            Serial_Printf("ESP8266 upload failed at %s\r\n", ESP8266_GetLastError());
                            Serial_Printf("ESP8266 rx:\r\n%s\r\n", ESP8266_GetRxBuffer());
                        }
                    }

                    vTaskDelay(pdMS_TO_TICKS(10000));
                }
                else
                {
                    Serial_Printf("ESP8266: heartbeat failed, reconnecting\r\n");
                    Serial_Printf("ESP8266 rx:\r\n%s\r\n", ESP8266_GetRxBuffer());
                    break;
                }
            }
        }
        else
        {
            Serial_Printf("ESP8266: WiFi join failed, status=%d\r\n", status);
            Serial_Printf("ESP8266 rx:\r\n%s\r\n", ESP8266_GetRxBuffer());
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

/* ========== Sensor_Task ========== */

/*
 * 负责：读 DHT11、读光敏、判断报警、控制蜂鸣器。
 * 读完之后把数据打包发进队列，通知 Display_Task 刷新显示。
 * 每 2 秒循环一次。
 */
static void Sensor_Task(void *pvParameters)
{
    SensorData_t data;
    TickType_t   xLastWakeTime;

    /*
     * 记录任务启动时的 Tick，配合 vTaskDelayUntil 实现精确 2 秒周期。
     * vTaskDelayUntil 比 vTaskDelay 更准确：它从上一次唤醒时刻算起，
     * 不会因为任务自身执行时间而累积误差。
     */
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        /* 读取 DHT11 */
        data.dht_ok = DHT11_Read_Data(&data.temp, &data.humi);

        /* 读取光敏模块 */
        data.light_do = LIGHT_GetDO();
        data.light_ad = AD_GetValue();

        /* 报警判断：温度或湿度超限 */
        if (data.dht_ok == 0 && (data.temp >= TEMP_MAX || data.humi >= HUMI_MAX))
        {
            data.alarm = 1;
            BEEP_ON();
        }
        else
        {
            data.alarm = 0;
            BEEP_OFF();
        }

        /*
         * 把数据发入队列。
         * xQueueOverwrite 会直接覆盖队列里已有的旧数据，
         * 保证 Display_Task 总能拿到最新的传感器数据，不会积压。
         */
        xQueueOverwrite(xSensorQueue, &data);
        xQueueOverwrite(xCloudQueue, &data);

        /* 精确等待到下一个 2 秒周期点 */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(2000));
    }
}

/* ========== Display_Task ========== */

/*
 * 负责：从队列取数据，刷新 OLED，输出串口日志。
 * 阻塞等待队列有数据，收到后立即刷新，然后再等。
 */
static void Display_Task(void *pvParameters)
{
    SensorData_t data;

    while (1)
    {
        /*
         * 阻塞等待队列数据，最多等 3 秒（portMAX_DELAY 表示永远等）。
         * 用 3 秒超时是为了防止 Sensor_Task 异常挂死时 OLED 一直不更新。
         */
        if (xQueueReceive(xSensorQueue, &data, pdMS_TO_TICKS(3000)) == pdTRUE)
        {
            OLED_Clear();

            if (data.dht_ok == 0)
            {
                /* 第 1 行：温度 */
                OLED_ShowString(1, 1, "Temp:");
                OLED_ShowNum(1, 6, data.temp, 2);
                OLED_ShowString(1, 8, "C");

                /* 第 2 行：湿度 */
                OLED_ShowString(2, 1, "Humi:");
                OLED_ShowNum(2, 6, data.humi, 2);
                OLED_ShowString(2, 8, "%");

                /* 第 3 行：报警状态 */
                if (data.alarm)
                {
                    OLED_ShowString(3, 1, "Alarm! ");
                }
                else
                {
                    OLED_ShowString(3, 1, "Normal ");
                }
            }
            else
            {
                OLED_ShowString(1, 1, "DHT11 Error");
            }

            /* 第 4 行：光敏状态 + ADC 值 */
            OLED_ShowString(4, 1, "L:");
            if (data.light_do == 1)
            {
                OLED_ShowString(4, 3, "Dark  ");
            }
            else
            {
                OLED_ShowString(4, 3, "Bright");
            }
            OLED_ShowString(4, 10, "A:");
            OLED_ShowNum(4, 12, data.light_ad, 4);

            /* 串口输出 */
            if (data.dht_ok == 0)
            {
                Serial_Printf("Temp:%dC Humi:%d%% Light:%s AD:%d Alarm:%d\r\n",
                              data.temp,
                              data.humi,
                              data.light_do ? "Dark" : "Bright",
                              data.light_ad,
                              data.alarm);
            }
            else
            {
                Serial_Printf("DHT11 Error, Light:%s AD:%d\r\n",
                              data.light_do ? "Dark" : "Bright",
                              data.light_ad);
            }
        }
        else
        {
            /* 超时 3 秒没收到数据，说明 Sensor_Task 可能异常 */
            OLED_Clear();
            OLED_ShowString(1, 1, "Sensor Timeout");
            Serial_Printf("Sensor Task Timeout!\r\n");
        }
    }
}
