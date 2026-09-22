/*
 * 03_bno055 — BNO055 I2C 测试（BSP 重构版）
 *
 * 流程：
 *   1. 初始化 USART + BNO055
 *   2. 探测芯片 ID（应读 0xA0）
 *   3. 每 100ms 读一次欧拉角，串口打印
 */
#include <stdint.h>
#include "system_stm32f1xx.h"
#include "hal_delay.h"
#include "hal_usart.h"
#include "drv_bno055.h"

int main(void) {
    SystemInit();
    delay_init();
    usart1_init_115200();

    usart1_puts("\r\n=== BNO055 Driver (BSP refactored) ===\r\n");
    usart1_puts("Phase B.2.2 - register-level I2C\r\n");
    usart1_puts("SCL=PB6 SDA=PB7 (4.7k pull-up)\r\n\r\n");

    /* BNO055 初始化在 bsp_bno055_init() 里做（含 I2C + GPIO）*/
    if (!bno055_init()) {
        usart1_puts("ERROR: BNO055 not detected (check I2C wiring)\r\n");
        while (1) {
            delay_ms(1000);
            usart1_puts("[retry] BNO055 not found\r\n");
        }
    }
    usart1_puts("BNO055 detected! NDOF mode enabled.\r\n\r\n");

    uint32_t count = 0;
    while (1) {
        bno055_euler_t eul;
        if (bno055_read_euler(&eul)) {
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] Y=");
            usart1_putu(eul.heading);
            usart1_puts(" R=");
            usart1_putu(eul.roll);
            usart1_puts(" P=");
            usart1_putu(eul.pitch);
            usart1_puts("\r\n");
        } else {
            usart1_puts("[");
            usart1_putu(count);
            usart1_puts("] read error\r\n");
        }
        count++;
        delay_ms(100);
    }
}
