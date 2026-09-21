/*
 * app_tasks.c — 5 任务实现 + IPC（queue_count + mutex_usart）+ log_msg
 *
 * 本文件依赖 FreeRTOS 和 bsp_usart（log_msg 通过它输出）。
 */
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "bsp_usart.h"
#include "bsp_led.h"
#include "app_tasks.h"

/* ===========================================================================
 * IPC 对象（模块内私有）
 * =========================================================================== */
static QueueHandle_t     queue_count  = NULL;
static SemaphoreHandle_t mutex_usart  = NULL;

/* ===========================================================================
 * log_msg — 受 mutex 保护的串口打印（替代直接调用 bsp_usart_puts）
 * =========================================================================== */
void log_msg(const char *task_name, const char *label, uint32_t val) {
    if (mutex_usart) {
        xSemaphoreTake(mutex_usart, pdMS_TO_TICKS(50));
    }
    bsp_usart_puts("[");
    bsp_usart_puts(task_name);
    bsp_usart_puts("] ");
    bsp_usart_puts(label);
    bsp_usart_putu(val);
    bsp_usart_puts("\r\n");
    if (mutex_usart) {
        xSemaphoreGive(mutex_usart);
    }
}

/* ===========================================================================
 * app_tasks_init — 创建队列和互斥锁（在 main 的 xTaskCreate 之前调用一次）
 * =========================================================================== */
void app_tasks_init(void) {
    queue_count = xQueueCreate(4, sizeof(uint32_t));
    mutex_usart = xSemaphoreCreateMutex();
    if (queue_count == NULL || mutex_usart == NULL) {
        bsp_usart_puts("[FATAL] app_tasks_init: IPC create failed!\r\n");
        for (;;) {}
    }
}

/* ===========================================================================
 * 5 个任务实现
 * =========================================================================== */

/* 蓝灯 5Hz 快闪 */
void Task_FastBlink(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();
    while (1) {
        bsp_led_blue_toggle();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(100));
    }
}

/* 红灯 1Hz 慢闪 */
void Task_SlowBlink(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWake = xTaskGetTickCount();
    while (1) {
        bsp_led_red_toggle();
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(500));
    }
}

/* 生产者：每 200ms 自增并入队 */
void Task_Producer(void *pvParameters) {
    (void)pvParameters;
    uint32_t count = 0;
    TickType_t xLastWake = xTaskGetTickCount();
    while (1) {
        count++;
        if (xQueueSend(queue_count, &count, 0) == pdTRUE) {
            log_msg("Producer", "send #", count - 1);
        } else {
            log_msg("Producer", "QUEUE FULL, drop #", count - 1);
        }
        vTaskDelayUntil(&xLastWake, pdMS_TO_TICKS(200));
    }
}

/* 消费者：阻塞等队列 + 串口打印 + 模拟 50ms 处理 */
void Task_Printer(void *pvParameters) {
    (void)pvParameters;
    uint32_t val;
    while (1) {
        if (xQueueReceive(queue_count, &val, portMAX_DELAY) == pdTRUE) {
            bsp_led_green_toggle();
            log_msg("Printer", "recv=", val);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/* 心跳：每 5s 打印队列占用 + 系统 tick */
void Task_Heartbeat(void *pvParameters) {
    (void)pvParameters;
    uint32_t hb_tick = 0;
    while (1) {
        TickType_t now  = xTaskGetTickCount();
        UBaseType_t qcnt = uxQueueMessagesWaiting(queue_count);

        if (mutex_usart) {
            xSemaphoreTake(mutex_usart, pdMS_TO_TICKS(50));
        }
        bsp_usart_puts("[HB #");
        bsp_usart_putu(hb_tick);
        bsp_usart_puts("] uptime=");
        bsp_usart_putu(now * portTICK_PERIOD_MS);
        bsp_usart_puts("ms queue=");
        bsp_usart_putu((uint32_t)qcnt);
        bsp_usart_puts("\r\n");
        if (mutex_usart) {
            xSemaphoreGive(mutex_usart);
        }

        hb_tick++;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
