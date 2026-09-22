/*
 * =============================================================================
 * 06_freertos_demo — main（执行序列）
 * =============================================================================
 *
 * 硬件初始化 → IPC → 任务 → 启动调度器。
 * 详细注释和任务说明见 docs/DESIGN.md（待补）。
 *
 * 文件清单：
 *   hal_usart.h/c   — USART1 驱动（纯硬件层）
 *   bsp_led.h/c     — RGB LED 驱动（纯硬件层）
 *   app_tasks.h/c   — 5 任务 + 队列 + 互斥锁 + log_msg（应用层）
 *   freertos_hooks.c— FreeRTOS 钩子（错误处理）
 *   main.c          — 本文件（执行序列）
 * =============================================================================
 */

#include "FreeRTOS.h"
#include "task.h"

#include "system_stm32f1xx.h"   /* SystemInit() — 72MHz 时钟 */

#include "hal_usart.h"
#include "bsp_led.h"
#include "app_tasks.h"

int main(void) {
    /* 1. 硬件初始化 */
    SystemInit();                 /* 72MHz 时钟（common/system_stm32f1xx.c）*/
    hal_usart_init();             /* USART1 @ 115200 8N1 */
    bsp_led_init();               /* RGB LED：红 PB5 / 绿 PB0 / 蓝 PB1 */

    /* 2. 启动横幅（用裸 hal_usart_puts，此时 mutex 还没建）*/
    hal_usart_puts("\r\n=== FreeRTOS 5-Task Demo ===\r\n");
    hal_usart_puts("Fast=Blink(prio2,100ms) Slow=Blink(prio1,500ms) ");
    hal_usart_puts("Producer(prio2,200ms) Printer(prio3,event) HB(idle+1,5s)\r\n\r\n");

    /* 3. 创建 IPC 对象（队列 + 互斥锁）*/
    app_tasks_init();

    /* 4. 创建任务（详见 app_tasks.h）*/
    xTaskCreate(Task_FastBlink, "Fast",      128, NULL, 2,                  NULL);
    xTaskCreate(Task_SlowBlink, "Slow",      128, NULL, 1,                  NULL);
    xTaskCreate(Task_Producer,  "Producer",  128, NULL, 2,                  NULL);
    xTaskCreate(Task_Printer,   "Printer",   256, NULL, 3,                  NULL);
    xTaskCreate(Task_Heartbeat, "Heartbeat", 128, NULL, tskIDLE_PRIORITY + 1, NULL);

    /* 5. 启动调度器（从此处不再返回）*/
    vTaskStartScheduler();

    /* 不应该到这里 */
    hal_usart_puts("[FATAL] Scheduler returned!\r\n");
    for (;;) {}
}
