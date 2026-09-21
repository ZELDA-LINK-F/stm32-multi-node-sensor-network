/*
 * dht11.c — DHT11 单总线驱动实现
 *
 * 协议时序：
 *   1. 主机起始：拉低 18ms，释放，等 30us
 *   2. DHT11 响应：拉低 80us，拉高 80us
 *   3. 数据 40 bit：每 bit = 50us 低 + (26us高=0 或 70us高=1)
 *   4. 字节序：湿度整数 / 湿度小数 / 温度整数 / 温度小数 / 校验
 *
 * 单总线需要：
 *   - 开漏输出（CNF=01）+ 50MHz（MODE=11）
 *   - 外部 4.7kΩ 上拉到 VCC（DHT11 模块通常已焊）
 */
#include "dht11.h"

/* === DWT 周期计数器做 us 延时（@ 72MHz = 72 cycles/us）===
 * 不依赖 SysTick（DWT 是 ARM 内核调试单元，永远在跑）
 */
static void delay_us(uint32_t us) {
    static uint8_t dwt_inited = 0;
    if (!dwt_inited) {
        /* 启用 DWT */
        *(volatile uint32_t *)0xE000EDFCUL |= (1U << 24);  /* DEMCR.TRCENA */
        *(volatile uint32_t *)0xE0001004UL = 0;              /* DWT.CYCCNT = 0 */
        *(volatile uint32_t *)0xE0001000UL |= (1U << 0);   /* DWT.CTRL.CYCCNTENA */
        dwt_inited = 1;
    }

    volatile uint32_t *cyccnt = (volatile uint32_t *)0xE0001004UL;
    uint32_t start = *cyccnt;
    uint32_t ticks = us * 72;  /* 72MHz = 72 cycles/us */
    while ((*cyccnt - start) < ticks);
}

/* === PA8 操作 === */
static inline void dht11_pin_low(void)  { DHT11_GPIO_BSRR_REG = (1U << (DHT11_PIN_NUM + 16)); }   /* reset bit */
static inline void dht11_pin_high(void) { DHT11_GPIO_BSRR_REG = (1U << DHT11_PIN_NUM); }           /* set bit */
static inline uint8_t dht11_pin_read(void) { return (DHT11_GPIO_IDR_REG >> DHT11_PIN_NUM) & 1U; }

/* 设为开漏输出 50MHz：CNF=01 MODE=11 → 0x7 */
static inline void dht11_set_output(void) {
    DHT11_GPIO_CRH_REG = (DHT11_GPIO_CRH_REG & ~(0xFU << 0)) | (0x7U << 0);
}

/* 设为浮空输入：CNF=01 MODE=00 → 0x4 */
static inline void dht11_set_input(void) {
    DHT11_GPIO_CRH_REG = (DHT11_GPIO_CRH_REG & ~(0xFU << 0)) | (0x4U << 0);
}

/* 等引脚到指定电平，超时返回 1 */
static uint8_t dht11_wait(uint8_t level, uint32_t timeout_us) {
    uint32_t t = 0;
    while (dht11_pin_read() != level) {
        if (++t > timeout_us) return 1;
        delay_us(1);
    }
    return 0;
}

/* === 初始化 === */
void dht11_init(void) {
    /* 开 GPIOA 时钟 */
    *(volatile uint32_t *)0x40021018 |= DHT11_RCC_IOPAEN_BIT;
    /* 设 PA8 开漏输出，默认高（总线释放）*/
    dht11_set_output();
    dht11_pin_high();

    /* 关键：把 SysTick 配成 1us/tick（默认 SystemInit 是 1ms/tick）*/
    *(volatile uint32_t *)0xE000E014UL = 72 - 1;       /* LOAD */
    *(volatile uint32_t *)0xE000E018UL = 0;             /* VAL */
    *(volatile uint32_t *)0xE000E010UL = 0x05;          /* CLKSOURCE | ENABLE */

    delay_us(1000);  /* 上电稳定 */
}

/* === 读 1 bit ===
 * 数据 bit 格式：50us 低 + 高电平（26-28us=0，70us=1）
 * 我们等下降沿开始，等高电平 40us 后采样
 *   - 如果还是高 → 1（继续等高电平结束）
 *   - 如果变低了 → 0
 */
static uint8_t dht11_read_bit(void) {
    uint8_t bit = 0;

    /* 等 50us 低电平结束（下一个 bit 开始）*/
    if (dht11_wait(1, 100)) return 0xFF;  /* 超时 */

    /* 等 30us 后采样 */
    delay_us(30);

    if (dht11_pin_read()) {
        /* 还是高 → 数据 1 */
        bit = 1;
        /* 等高电平结束（进入下一个 bit 的低电平）*/
        if (dht11_wait(0, 100)) return 0xFF;
    }
    /* 否则是 0，bit = 0 */

    return bit;
}

/* === 读 1 字节（高位在前）=== */
static uint8_t dht11_read_byte(void) {
    uint8_t byte = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b = dht11_read_bit();
        if (b == 0xFF) return 0xFF;
        byte = (byte << 1) | b;
    }
    return byte;
}

/* === 读一次数据（5 字节）=== */
uint8_t dht11_read(uint8_t *temp_int, uint8_t *temp_dec,
                   uint8_t *humi_int, uint8_t *humi_dec) {
    uint8_t data[5] = {0};

    /* 1. 主机起始信号：拉低 18ms，释放 */
    dht11_set_output();
    dht11_pin_low();
    delay_us(18000);
    dht11_pin_high();
    delay_us(30);  /* 20-40us 之间 */

    /* 2. 切输入模式等 DHT11 响应 */
    dht11_set_input();

    /* 3. DHT11 拉低 80us（响应）*/
    if (dht11_wait(0, 100)) return 1;  /* 超时 = DHT11 没响应 */

    /* 4. DHT11 拉高 80us（准备开始发数据）*/
    if (dht11_wait(1, 100)) return 1;

    /* 5. 读 5 字节（40 bit）*/
    for (uint8_t i = 0; i < 5; i++) {
        data[i] = dht11_read_byte();
        if (data[i] == 0xFF) return 1;  /* 读取超时 */
    }

    /* 6. 切回输出模式 */
    dht11_set_output();
    dht11_pin_high();

    /* 7. 校验和 = 前 4 字节之和的低 8 位 */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return 1;  /* 校验失败 */

    /* 8. 输出 */
    *humi_int = data[0];
    *humi_dec = data[1];
    *temp_int = data[2];
    *temp_dec = data[3];

    return 0;
}
