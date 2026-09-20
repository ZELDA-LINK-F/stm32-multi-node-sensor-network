/*
 * FreeRTOSConfig.h — STM32F103 + FreeRTOS V10.6.1 配置
 * 参考：https://www.freertos.org/a00110.html
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

/* === 基础配置 === */
#define configUSE_PREEMPTION            1   /* 抢占式调度 */
#define configUSE_IDLE_HOOK             0
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              ( 72000000UL )  /* SystemInit 后 72MHz */
#define configTICK_RATE_HZ              ( 1000 )          /* 1ms 心跳 */
#define configMAX_PRIORITIES            ( 5 )             /* 0-4 共 5 个优先级 */
#define configMINIMAL_STACK_SIZE        ( 128 )           /* Idle 任务栈 */
#define configMAX_TASK_NAME_LEN         ( 8 )
#define configUSE_TRACE_FACILITY        1
#define configUSE_16_BIT_TICKS          0   /* 用 32-bit tick */

/* === 内存管理 === */
#define configSUPPORT_DYNAMIC_ALLOCATION    1
#define configSUPPORT_STATIC_ALLOCATION     0
#define configTOTAL_HEAP_SIZE               ( 8 * 1024 )   /* 8KB heap_4 */

/* === 钩子 === */
#define configUSE_MALLOC_FAILED_HOOK      1
#define configUSE_STACK_OVERFLOW_HOOK     2   /* 方法 2：方法 1 太慢 */

/* === 协程（v10 已弃用，关闭）=== */
#define configUSE_CO_ROUTINES             0

/* === 互斥/信号量 === */
#define configUSE_MUTEXES                 1
#define configUSE_RECURSIVE_MUTEXES       1
#define configUSE_COUNTING_SEMAPHORES     1
#define configUSE_BINARY_SEMAPHORES       1

/* === 其他 === */
#define configUSE_TASK_NOTIFICATIONS      1
#define configUSE_QUEUE_SETS              0
#define configUSE_TICKLESS_IDLE           0   /* 第一版不省电 */
#define configCHECK_FOR_STACK_OVERFLOW    2

/* === 函数包含（必须定义） === */
#define INCLUDE_vTaskPrioritySet          1
#define INCLUDE_uxTaskPriorityGet         1
#define INCLUDE_vTaskDelete               1
#define INCLUDE_vTaskCleanUpResources     0
#define INCLUDE_vTaskSuspend              1
#define INCLUDE_vTaskDelayUntil           1
#define INCLUDE_vTaskDelay                1
#define INCLUDE_xTaskGetSchedulerState    1

/* === Cortex-M3 特定 === */
#define configKERNEL_INTERRUPT_PRIORITY   15  /* 最低，配置 SysTick/PendSV */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 5  /* FromISR 函数可用的最高优先级 */

/* === 中断优先级包装（STM32 HAL 风格 4-bit 优先级） === */
/* 注意：我们不用 HAL，但 NVIC_SetPriority API 接受 4-bit 优先级 */
#define configPRIO_BITS                   4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5

/* === 断言失败回调（调 usart1 输出）=== */
void vAssertCalled(const char *pcFile, uint32_t ulLine);
#define configASSERT( x )  if( ( x ) == 0 ) vAssertCalled(__FILE__, __LINE__)

/* 钩子函数在 main.c 中定义，无需前置声明 */

#endif /* FREERTOS_CONFIG_H */
