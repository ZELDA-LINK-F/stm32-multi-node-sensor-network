/*
 * 05_freertos — FreeRTOS 4 任务并行（BSP 重构版）
 *
 * 4 任务架构：
 *   Task_Sensor    (prio 3): 仿真 BNO055 100Hz 数据
 *   Task_Protocol  (prio 2): 协议打包
 *   Task_TX        (prio 1): 串口打印
 *   Task_Heartbeat (prio 0): 5s 心跳 + LED 翻转
 *
 * LED = PB0（野火指南者绿灯确认 pin）
 */
#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "system_stm32f1xx.h"
#include "hal_delay.h"
#include "hal_gpio.h"
#include "hal_usart.h"
#include "protocol.h"

#define LED_PORT  GPIO_PORT_B
#define LED_PIN   0

/* === LED 控制 === */
static void led_init(void) {
    gpio_enable_clock(LED_PORT);
    gpio_config_pin(LED_PORT, LED_PIN, GPIO_MODE_OUTPUT_PP_50M);
    gpio_set_pin(LED_PORT, LED_PIN, 1);  /* 默认灭（低电平点亮）*/
}

static void led_toggle(void) {
    /* 读当前值 → 翻转 → 写回（避免直接 XOR 因为读 BSRR 不便）*/
    
    gpio_toggle_pin(LED_PORT, LED_PIN);
}

/* === 消息结构 === */
typedef struct {
    uint32_t timestamp;
    uint8_t  source;
    uint8_t  data[16];
    uint8_t  data_len;
} sensor_msg_t;

typedef struct {
    uint8_t  frame[32];
    uint8_t  frame_len;
} tx_msg_t;

static QueueHandle_t queue_data = NULL;
static QueueHandle_t queue_tx   = NULL;

/* === Task_Sensor: 100Hz 仿真 === */
static void Task_Sensor(void *p) {
    sensor_msg_t msg;
    uint32_t tick = 0;
    while (1) {
        msg.source = 2;
        msg.data_len = 6;
        msg.timestamp = tick * 10;
        msg.data[0] = (uint8_t)((tick * 30) >> 8);
        msg.data[1] = (uint8_t)((tick * 30) & 0xFF);
        msg.data[2] = (uint8_t)((tick * 5 + 100) >> 8);
        msg.data[3] = (uint8_t)((tick * 5 + 100) & 0xFF);
        msg.data[4] = (uint8_t)((tick * 2 + 50) >> 8);
        msg.data[5] = (uint8_t)((tick * 2 + 50) & 0xFF);

        /* 每 10 个 tick（1s）翻一次 LED */
        if ((tick % 10) == 0) led_toggle();

        xQueueSend(queue_data, &msg, 0);
        tick++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* === Task_Protocol: 打包协议帧 === */
static uint8_t crc16_simple(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0xA001;
            else crc <<= 1;
        }
    }
    return crc;
}

static void Task_Protocol(void *p) {
    sensor_msg_t in;
    tx_msg_t out;
    while (1) {
        if (xQueueReceive(queue_data, &in, portMAX_DELAY) == pdTRUE) {
            out.frame[0] = 0xAA;
            out.frame[1] = 0x55;
            out.frame[2] = 5 + in.data_len;
            out.frame[3] = 0x01;
            out.frame[4] = in.source;
            memcpy(&out.frame[5], in.data, in.data_len);
            uint16_t crc = crc16_simple(&out.frame[2], 3 + in.data_len);
            out.frame[5 + in.data_len]     = crc & 0xFF;
            out.frame[5 + in.data_len + 1] = crc >> 8;
            out.frame_len = 7 + in.data_len;

            xQueueSend(queue_tx, &out, 0);
        }
    }
}

/* === Task_TX: 串口打印协议帧 === */
static void Task_TX(void *p) {
    tx_msg_t msg;
    while (1) {
        if (xQueueReceive(queue_tx, &msg, portMAX_DELAY) == pdTRUE) {
            usart1_puts("[");
            usart1_putu(xTaskGetTickCount());
            usart1_puts("] ");
            for (uint8_t i = 0; i < msg.frame_len; i++) {
                uint8_t b = msg.frame[i];
                const char hex[] = "0123456789ABCDEF";
                usart1_putc(hex[b >> 4]);
                usart1_putc(hex[b & 0xF]);
                usart1_putc(' ');
            }
            usart1_puts("\r\n");
            delay_ms(5);  /* 模拟发送耗时 */
        }
    }
}

/* === Task_Heartbeat: 心跳 === */
static void Task_Heartbeat(void *p) {
    uint32_t hb = 0;
    while (1) {
        usart1_puts("[HB ");
        usart1_putu(hb);
        usart1_puts("] queue_data=");
        usart1_putu(uxQueueMessagesWaiting(queue_data));
        usart1_puts(" queue_tx=");
        usart1_putu(uxQueueMessagesWaiting(queue_tx));
        usart1_puts("\r\n");
        hb++;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

int main(void) {
    SystemInit();
    delay_init();
    usart1_init_115200();
    led_init();

    queue_data = xQueueCreate(8, sizeof(sensor_msg_t));
    queue_tx   = xQueueCreate(4, sizeof(tx_msg_t));

    xTaskCreate(Task_Sensor,    "Sensor",    256, NULL, 3, NULL);
    xTaskCreate(Task_Protocol,  "Protocol",  256, NULL, 2, NULL);
    xTaskCreate(Task_TX,        "TX",        256, NULL, 1, NULL);
    xTaskCreate(Task_Heartbeat, "Heartbeat", 256, NULL, 0, NULL);

    vTaskStartScheduler();
    while (1);
}

/* === FreeRTOS 钩子函数（必须实现）=== */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    usart1_puts("[FATAL] Stack overflow: ");
    usart1_puts(pcTaskName);
    usart1_puts("\r\n");
    for (;;);
}

void vApplicationMallocFailedHook(void) {
    usart1_puts("[FATAL] Malloc failed\r\n");
    for (;;);
}

void vAssertCalled(const char *pcFile, uint32_t ulLine) {
    usart1_puts("[ASSERT] ");
    usart1_puts(pcFile);
    usart1_puts(":");
    usart1_putu(ulLine);
    usart1_puts("\r\n");
    for (;;);
}
