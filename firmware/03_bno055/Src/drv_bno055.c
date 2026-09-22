/*
 * bsp_bno055.c — BNO055 应用层
 */
#include "drv_bno055.h"
#include "drv_i2c.h"

/* === SysTick 延时（复用 bsp_i2c1.c 里的，但独立声明）=== */
#define SysTick_LOAD     (*(volatile uint32_t *)0xE000E014UL)
#define SysTick_VAL      (*(volatile uint32_t *)0xE000E018UL)
#define SysTick_CTRL     (*(volatile uint32_t *)0xE000E010UL)

static void delay_us(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        SysTick_VAL = 0;
        while (!(SysTick_CTRL & (1U << 16)));
    }
}

static void delay_ms(uint32_t ms) {
    while (ms--) delay_us(1000);
}

/* 写 BNO055 1 字节寄存器（应用层函数）*/
static uint8_t _priv_bno055_write_reg(uint8_t reg, uint8_t data) {
    if (!i2c1_start()) return 0;
    if (!i2c1_send_byte(BNO055_I2C_ADDR << 1 | 0)) { i2c1_stop(); return 0; }
    if (!i2c1_send_byte(reg)) { i2c1_stop(); return 0; }
    if (!i2c1_send_byte(data)) { i2c1_stop(); return 0; }
    i2c1_stop();
    return 1;
}

/* 读 BNO055 1 字节寄存器（应用层函数）*/
static uint8_t _priv_bno055_read_reg(uint8_t reg, uint8_t *data) {
    /* 第一阶段：写寄存器地址 */
    if (!i2c1_start()) return 0;
    if (!i2c1_send_byte(BNO055_I2C_ADDR << 1 | 0)) { i2c1_stop(); return 0; }
    if (!i2c1_send_byte(reg)) { i2c1_stop(); return 0; }

    /* 第二阶段：重启 + 读 */
    if (!i2c1_start()) return 0;
    if (!i2c1_send_byte(BNO055_I2C_ADDR << 1 | 1)) { i2c1_stop(); return 0; }
    *data = i2c1_receive_byte(0);  /* 最后一字节发 NACK */
    i2c1_stop();
    return 1;
}

/* 连续读 N 字节（BNO055 自动地址递增）*/
static uint8_t _priv_bno055_read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    if (!i2c1_start()) return 0;
    if (!i2c1_send_byte(BNO055_I2C_ADDR << 1 | 0)) { i2c1_stop(); return 0; }
    if (!i2c1_send_byte(reg)) { i2c1_stop(); return 0; }
    if (!i2c1_start()) return 0;
    if (!i2c1_send_byte(BNO055_I2C_ADDR << 1 | 1)) { i2c1_stop(); return 0; }

    for (uint8_t i = 0; i < len - 1; i++) {
        buf[i] = i2c1_receive_byte(1);  /* ACK */
    }
    buf[len - 1] = i2c1_receive_byte(0);  /* NACK，最后一字节 */
    i2c1_stop();
    return 1;
}

/* BNO055 初始化 */
uint8_t bno055_init(void) {
    uint8_t chip_id = 0;

    /* 1. 探测设备（读 CHIP_ID 寄存器 0x00 应返回 0xA0）*/
    if (!_priv_bno055_read_reg(BNO055_REG_CHIP_ID, &chip_id)) return 0;
    if (chip_id != 0xA0) return 0;

    /* 2. 配置 NDOF 模式 */
    if (!_priv_bno055_write_reg(BNO055_REG_OPR_MODE, BNO055_OPR_NDOF)) return 0;
    delay_ms(10);  /* 模式切换需要时间 */

    return 1;
}

/* 读欧拉角（heading/roll/pitch，单位 1/16 度）*/
uint8_t bno055_read_euler(bno055_euler_t *eul) {
    uint8_t buf[6];

    /* 寄存器 0x1A-0x1F：HEADING_MSB HEADING_LSB ROLL_MSB ROLL_LSB PITCH_MSB PITCH_LSB */
    if (!_priv_bno055_read_regs(BNO055_REG_EUL_H_MSB, buf, 6)) return 0;

    /* 解析为有符号 16 位整数（小端序）*/
    eul->heading = (int16_t)(buf[0] | (buf[1] << 8));
    eul->roll    = (int16_t)(buf[2] | (buf[3] << 8));
    eul->pitch   = (int16_t)(buf[4] | (buf[5] << 8));

    return 1;
}
