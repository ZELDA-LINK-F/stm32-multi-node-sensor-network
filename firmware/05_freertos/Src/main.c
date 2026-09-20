/*
 * 05_freertos — FreeRTOS 4 任务并行（B.3）
 *
 * 架构：
 *   Task Sensor (prio 3):  每 10ms 读 BNO055（仿真）→ 入队 queue_data
 *   Task Protocol (prio 2): 阻塞等队列 → 打包成帧（仿真）→ 入队 queue_tx
 *   Task TX (prio 1):       阻塞等队列 → 打印（仿真 NRF24）
 *   Task Heartbeat (prio 0): 每 5s 打印一次 "HB"
 *
 * v0.1 简化：
 *   - BNO055/DHT11 读用计数模拟（不接真实硬件）
 *   - NRF24 发送用串口打印代替
 *   - 完整流程可见：4 任务切换 + 队列通信
 *
 * 简历可写：
 *   "FreeRTOS V10.6.1 原生 API 移植 STM32F103，4 任务并行 + 2 队列通信，
 *    configUSE_PREEMPTION=1 抢占式调度"
 */

#include <stdint.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* LED 闪烁前向声明 */
static void led_init(void);
static void led_toggle_all(void);
#include "system_stm32f1xx.h"

/* === USART1 (PA9/PA10 @ 115200) === */
#define USART1_BASE        0x40013800UL
#define USART1_SR          (*(volatile uint32_t *)(USART1_BASE + 0x00))
#define USART1_DR          (*(volatile uint32_t *)(USART1_BASE + 0x04))
#define USART1_BRR         (*(volatile uint32_t *)(USART1_BASE + 0x08))
#define USART1_CR1         (*(volatile uint32_t *)(USART1_BASE + 0x0C))

#define USART_SR_TXE       (1U << 7)
#define USART_CR1_UE       (1U << 13)
#define USART_CR1_TE       (1U << 3)
#define USART_CR1_RE       (1U << 2)

#define USART1_BRR_115200_72MHZ  ((39U << 4) | 1U)

static void usart1_init(void) {
    *(volatile uint32_t *)0x40021018 |= (1U << 2) | (1U << 14);
    *(volatile uint32_t *)0x40010804 =
        (*(volatile uint32_t *)0x40010804 & ~((0xFU << 4) | (0xFU << 8)))
        | ((0xBU << 4) | (0x4U << 8));
    USART1_BRR = USART1_BRR_115200_72MHZ;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void usart1_putc(char c) {
    while (!(USART1_SR & USART_SR_TXE));
    USART1_DR = (uint32_t)c;
}

static void usart1_puts(const char *s) { while (*s) usart1_putc(*s++); }

static void usart1_putu(uint32_t v) {
    char buf[11]; int i = 0;
    if (v == 0) { usart1_putc('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i) usart1_putc(buf[--i]);
}

/* === FreeRTOS 钩子（在 FreeRTOSConfig.h 里启用）=== */
void vApplicationMallocFailedHook(void) {
    usart1_puts("[FATAL] pvPortMalloc failed!\r\n");
    for (;;) {}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    usart1_puts("[FATAL] Stack overflow in task: ");
    usart1_puts(pcTaskName);
    usart1_puts("\r\n");
    for (;;) {}
}

void vAssertCalled(const char *pcFile, uint32_t ulLine) {
    usart1_puts("[ASSERT] ");
    usart1_puts(pcFile);
    usart1_puts(":");
    usart1_putu(ulLine);
    usart1_puts("\r\n");
    for (;;) {}
}

/* === 消息结构 === */
typedef struct {
    uint32_t timestamp;
    uint8_t  source;       /* 1=DHT11, 2=BNO055, 3=DS18B20 */
    uint8_t  data[16];
    uint8_t  data_len;
} sensor_msg_t;

typedef struct {
    uint8_t  frame[32];
    uint8_t  frame_len;
} tx_msg_t;

/* === 全局队列句柄 === */
static QueueHandle_t queue_data = NULL;
static QueueHandle_t queue_tx   = NULL;

/* ============================================================
 * Task_Sensor — 高频采集（仿真）
 * ============================================================ */
static void Task_Sensor(void *pvParameters) {
    (void)pvParameters;
    sensor_msg_t msg;
    uint32_t tick = 0;

    while (1) {
        /* BNO055: 每 10ms 一次（仿真 100Hz）*/
        msg.source = 2;  /* BNO055 */
        msg.data_len = 6;
        msg.timestamp = tick * 10;
        /* 仿真 yaw/roll/pitch 随 tick 变化 */
        msg.data[0] = (uint8_t)((tick * 30) >> 8);       /* yaw_msb */
        msg.data[1] = (uint8_t)((tick * 30) & 0xFF);     /* yaw_lsb */
        msg.data[2] = (uint8_t)((tick * 5 + 100) >> 8);  /* roll_msb */
        msg.data[3] = (uint8_t)((tick * 5 + 100) & 0xFF);
        msg.data[4] = (uint8_t)((tick * 2 + 50) >> 8);   /* pitch_msb */
        msg.data[5] = (uint8_t)((tick * 2 + 50) & 0xFF);

        if (xQueueSend(queue_data, &msg, 0) != pdTRUE) {
            usart1_puts("[Sensor] queue_data FULL!\r\n");
        }

        /* DHT11: 每 100 次循环 = 1Hz（仿真）*/
        if ((tick % 100) == 0) {
            msg.source = 1;
            msg.data_len = 2;
            msg.data[0] = 25;  /* 25℃ */
            msg.data[1] = 60;  /* 60% */
            xQueueSend(queue_data, &msg, 0);
        }

        /* LED 心跳：每 10 个 tick (100ms) 翻转一次 → 视觉 5Hz 闪烁 */
        if ((tick % 10) == 0) led_toggle_all();

        tick++;
        vTaskDelay(pdMS_TO_TICKS(10));   /* 100Hz */
    }
}

/* ============================================================
 * Task_Protocol — 打包成协议帧（CRC 简化版 = XOR）
 * ============================================================ */
static uint16_t crc16_simple(const uint8_t *data, uint8_t len) {
    /* 简化：用 XOR 占位（v0.2 用真正的 CRC16-MODBUS）*/
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) crc = (crc << 1) ^ 0xA001;
            else              crc <<= 1;
        }
    }
    return crc;
}

static void Task_Protocol(void *pvParameters) {
    (void)pvParameters;
    sensor_msg_t in;
    tx_msg_t out;

    while (1) {
        if (xQueueReceive(queue_data, &in, portMAX_DELAY) == pdTRUE) {
            /* 构造协议帧 (PROTOCOL.md v0.1) */
            out.frame[0] = 0xAA;
            out.frame[1] = 0x55;
            out.frame[2] = 5 + in.data_len;       /* LEN */
            out.frame[3] = 0x01;                  /* TYPE = DATA_REPORT */
            out.frame[4] = in.source;             /* DST_ID = source */
            memcpy(&out.frame[5], in.data, in.data_len);

            /* CRC16 over [LEN..PAYLOAD] */
            uint16_t crc = crc16_simple(&out.frame[2], 3 + in.data_len);
            out.frame[5 + in.data_len]     = crc & 0xFF;        /* CRC_L */
            out.frame[5 + in.data_len + 1] = (crc >> 8) & 0xFF; /* CRC_H */
            out.frame_len = 7 + in.data_len;

            xQueueSend(queue_tx, &out, 0);
        }
    }
}

/* ============================================================
 * Task_TX — 发送（仿真：串口打印）
 * ============================================================ */
static void Task_TX(void *pvParameters) {
    (void)pvParameters;
    tx_msg_t msg;

    while (1) {
        if (xQueueReceive(queue_tx, &msg, portMAX_DELAY) == pdTRUE) {
            /* 仿真 NRF24 发送：串口打印 HEX */
            usart1_puts("[TX] ");
            for (uint8_t i = 0; i < msg.frame_len; i++) {
                uint8_t b = msg.frame[i];
                usart1_putc("0123456789ABCDEF"[b >> 4]);
                usart1_putc("0123456789ABCDEF"[b & 0x0F]);
                usart1_putc(' ');
            }
            usart1_puts("\r\n");

            /* 仿真 NRF24 发送耗时 5ms */
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

/* ============================================================
 * Task_Heartbeat — 心跳（每 5s）
 * ============================================================ */
static void Task_Heartbeat(void *pvParameters) {
    (void)pvParameters;
    uint32_t hb = 0;

    while (1) {
        usart1_puts("[HB ");
        usart1_putu(hb);
        usart1_puts("] FreeRTOS alive. queue_data=");
        usart1_putu(uxQueueMessagesWaiting(queue_data));
        usart1_puts(" queue_tx=");
        usart1_putu(uxQueueMessagesWaiting(queue_tx));
        usart1_puts("\r\n");
        hb++;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ============================================================
 * main
 * ============================================================ */

/* RGB LED 配置（野火指南者：PB5=红 PB0=绿 PB1=蓝，共阴极，低电平点亮）*/
#define GPIOB_CRL (*(volatile uint32_t *)0x40010C00UL)
#define GPIOB_CRH (*(volatile uint32_t *)0x40010C04UL)
#define GPIOB_ODR (*(volatile uint32_t *)0x40010C0CUL)
static void led_init(void) {
    /* 1. 开 GPIOB 时钟 (RCC_APB2ENR bit3) */
    *(volatile uint32_t *)0x40021018 |= (1U << 3);

    /* 2. PB5(红) PB0(绿) PB1(蓝) 都配推挽输出 50MHz
     *    CNF=00 MODE=11 → 0x3
     *    PB0 在 CRL 的 bit[0:3]，PB1 在 bit[4:7]，PB5 在 bit[20:23]
     */
    /* PB0/1/5 都配推挽输出 50MHz，不管哪个版本都能覆盖 */
    GPIOB_CRL = (GPIOB_CRL & ~((0xFU << 0) | (0xFU << 4))) | ((0x3U << 0) | (0x3U << 4));
    GPIOB_CRH = (GPIOB_CRH & ~(0xFU << 20)) | (0x3U << 20);

    /* 3. 默认高电平（LED 全灭）*/
    GPIOB_ODR |= (1U << 0) | (1U << 1) | (1U << 5);
}

/* 3 个 LED 一起翻转 - 不管引脚映射如何都看得到闪 */
static void led_toggle_all(void) {
    GPIOB_ODR ^= (1U << 0) | (1U << 1) | (1U << 5);
}

int main(void) {
    SystemInit();           /* 72MHz 时钟 */
    usart1_init();
    led_init();
    led_toggle_all();  /* 第一次切换证明 SystemInit 成功 */

    usart1_puts("\r\n=== FreeRTOS 4-Task Demo (B.3) ===\r\n");
    usart1_puts("Task Sensor(3) / Protocol(2) / TX(1) / Heartbeat(0)\r\n");
    usart1_puts("Queue queue_data(8) / queue_tx(4)\r\n\r\n");

    /* 创建队列 */
    queue_data = xQueueCreate(8, sizeof(sensor_msg_t));
    queue_tx   = xQueueCreate(4, sizeof(tx_msg_t));

    if (queue_data == NULL || queue_tx == NULL) {
        usart1_puts("[FATAL] Queue creation failed!\r\n");
        for (;;) {}
    }

    /* 创建任务 */
    if (xTaskCreate(Task_Sensor,    "Sensor",    256, NULL, 3, NULL) != pdPASS ||
        xTaskCreate(Task_Protocol,  "Protocol",  256, NULL, 2, NULL) != pdPASS ||
        xTaskCreate(Task_TX,        "TX",        256, NULL, 1, NULL) != pdPASS ||
        xTaskCreate(Task_Heartbeat, "Heartbeat", 128, NULL, 0, NULL) != pdPASS) {
        usart1_puts("[FATAL] Task creation failed!\r\n");
        for (;;) {}
    }

    /* 启动调度器（从此处不再返回）*/
    vTaskStartScheduler();

    /* 不应该到达这里 */
    usart1_puts("[FATAL] Scheduler returned!\r\n");
    for (;;) {}
}
