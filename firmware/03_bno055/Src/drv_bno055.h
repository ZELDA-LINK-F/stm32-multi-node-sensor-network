/*
 * bsp_bno055.h — BNO055 应用层（基于 bsp_i2c1）
 */
#ifndef BSP_BNO055_H
#define BSP_BNO055_H

#include <stdint.h>

#define BNO055_I2C_ADDR      0x28   /* ADR=GND 默认地址 */

#define BNO055_REG_CHIP_ID   0x00   /* 应读 0xA0 */
#define BNO055_REG_OPR_MODE  0x3D
#define BNO055_REG_EUL_H_MSB 0x1A

#define BNO055_OPR_NDOF      0x0C   /* 九轴融合 */

typedef struct {
    int16_t heading;   /* yaw, 单位 1/16 度 */
    int16_t roll;
    int16_t pitch;
} bno055_euler_t;

uint8_t bno055_init(void);
uint8_t bno055_read_euler(bno055_euler_t *eul);

#endif
