/*
 * freertos_hooks.c — FreeRTOS 钩子函数（错误处理 + 调试）
 *
 * 触发条件由 FreeRTOSConfig.h 控制：
 *   - configUSE_MALLOC_FAILED_HOOK  →  vApplicationMallocFailedHook
 *   - configCHECK_FOR_STACK_OVERFLOW (方法 2) → vApplicationStackOverflowHook
 *   - configASSERT(x) 宏展开调用  →  vAssertCalled
 */
#include "FreeRTOS.h"
#include "task.h"

#include "hal_usart.h"

/* 栈溢出：打印任务名 + 死循环（OpenOCD 抓现场看 PC） */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    hal_usart_puts("\r\n[FATAL] STACK OVERFLOW in task: ");
    hal_usart_puts(pcTaskName);
    hal_usart_puts("\r\n");
    for (;;) { /* trap */ }
}

/* pvPortMalloc 失败：heap 不够 */
void vApplicationMallocFailedHook(void) {
    hal_usart_puts("\r\n[FATAL] pvPortMalloc failed (heap OOM)!\r\n");
    for (;;) { /* trap */ }
}

/* configASSERT 失败（在 FreeRTOSConfig.h 中由宏展开调用）*/
void vAssertCalled(const char *pcFile, uint32_t ulLine) {
    hal_usart_puts("\r\n[ASSERT] ");
    hal_usart_puts(pcFile);
    hal_usart_puts(":");
    hal_usart_putu(ulLine);
    hal_usart_puts("\r\n");
    for (;;) { /* trap */ }
}
