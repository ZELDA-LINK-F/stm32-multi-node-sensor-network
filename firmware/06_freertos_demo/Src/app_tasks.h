/*
 * app_tasks.h — 应用层：5 个任务 + IPC + 受 mutex 保护的串口打印
 *
 * 任务一览（详细说明见 main.c 顶部注释）：
 *   Task_FastBlink  prio=2  100ms   蓝灯 5Hz
 *   Task_SlowBlink  prio=1  500ms   红灯 1Hz
 *   Task_Producer   prio=2  200ms   自增 + 入队
 *   Task_Printer    prio=3  事件    出队 + 串口
 *   Task_Heartbeat  prio=tskIDLE+1  5s  统计
 */
#ifndef APP_TASKS_H
#define APP_TASKS_H

#include <stdint.h>

/* 任务函数（由 main.c 传给 xTaskCreate）*/
void Task_FastBlink(void *pvParameters);
void Task_SlowBlink(void *pvParameters);
void Task_Producer(void *pvParameters);
void Task_Printer(void *pvParameters);
void Task_Heartbeat(void *pvParameters);

/* 创建 IPC 对象（队列 + 互斥锁），在 xTaskCreate 之前调用一次 */
void app_tasks_init(void);

/* 受 mutex 保护的串口打印：格式 "[task] label<value>\r\n" */
void log_msg(const char *task_name, const char *label, uint32_t val);

#endif
