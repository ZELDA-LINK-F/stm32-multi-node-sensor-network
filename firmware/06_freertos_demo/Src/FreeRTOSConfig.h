/*
 * FreeRTOSConfig.h — STM32F103VET6 + FreeRTOS V10.6.1 配置
 *
 * 参照 05_freertos 版本，本 Demo 独立可编译。
 * 重点参数说明见各行注释。
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* === 基础调度 === */
#define configUSE_PREEMPTION            1     /* 1=抢占式（推荐）；0=合作式 */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              ( 72000000UL )
#define configTICK_RATE_HZ              ( 1000 )       /* SysTick 1ms */
#define configMAX_PRIORITIES            ( 6 )          /* 0~5，本 Demo 用 0~3 */
#define configMINIMAL_STACK_SIZE        ( 128 )        /* Idle 栈（字）*/
#define configMAX_TASK_NAME_LEN         ( 12 )
#define configUSE_TRACE_FACILITY        1              /* 启用 vTaskList 等 */
#define configUSE_16_BIT_TICKS          0              /* 32-bit tick */

/* === 内存管理 === */
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     0
#define configTOTAL_HEAP_SIZE               ( 8 * 1024 )  /* 8KB heap_4 */

/* === 钩子 === */
#define configUSE_MALLOC_FAILED_HOOK      1              /* malloc 失败回调 */
#define configUSE_STACK_OVERFLOW_HOOK     2              /* 方法 2 更可靠 */

/* === 协程 v10 已弃用 === */
#define configUSE_CO_ROUTINES             0

/* === IPC 开关（按需开启省 RAM）=== */
#define configUSE_MUTEXES                 1
#define configUSE_RECURSIVE_MUTEXES       0
#define configUSE_COUNTING_SEMAPHORES     1
#define configUSE_BINARY_SEMAPHORES       1

/* === 其他特性 === */
#define configUSE_TASK_NOTIFICATIONS      1              /* 任务通知，最快 IPC */
#define configUSE_QUEUE_SETS              0
#define configUSE_TICKLESS_IDLE           0              /* 本 Demo 不省电 */

/* === 可选 API 开关（1=包含代码）=== */
#define INCLUDE_vTaskPrioritySet          1
#define INCLUDE_uxTaskPriorityGet         1
#define INCLUDE_vTaskDelete               1
#define INCLUDE_vTaskCleanUpResources     0
#define INCLUDE_vTaskSuspend              1
#define INCLUDE_vTaskDelayUntil           1
#define INCLUDE_vTaskDelay                1
#define INCLUDE_xTaskGetSchedulerState    1

/* === Cortex-M3 中断优先级 === */
#define configPRIO_BITS                   4              /* STM32F103 是 4 位 */
#define configKERNEL_INTERRUPT_PRIORITY           ( 15 << (8 - configPRIO_BITS) )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY      ( 5  << (8 - configPRIO_BITS) )
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY          configKERNEL_INTERRUPT_PRIORITY
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    configMAX_SYSCALL_INTERRUPT_PRIORITY

/* === configASSERT 失败回调（main.c 中实现）=== */
void vAssertCalled(const char *pcFile, uint32_t ulLine);
#define configASSERT( x )                           \
    do {                                            \
        if ((x) == 0) vAssertCalled(__FILE__, __LINE__); \
    } while(0)

#endif /* FREERTOS_CONFIG_H */
