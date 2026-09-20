/*
 * system_stm32f1xx.c — STM32F103 寄存器级 SystemInit
 *
 * 作用：上电后把系统时钟从默认 8MHz HSI 切到 72MHz（HSE × 9 PLL）
 * 参考：RM0008 第 7 章 Reset and Clock Control (RCC)
 *
 * 时钟树（最终状态）：
 *   HSE 8MHz ──► PLL × 9 ──► SYSCLK 72MHz
 *                              ├── AHB  /1 → HCLK  72MHz
 *                              ├── APB1 /2 → PCLK1 36MHz（I2C/USART2/3）
 *                              └── APB2 /1 → PCLK2 72MHz（USART1/GPIOA）
 *
 * 调用顺序（startup_stm32f103vctx.s 内）：
 *   Reset_Handler → SystemInit() → main()
 */

#include "system_stm32f1xx.h"

/* ============================================================
 * 寄存器基址（RM0008 表 1 Memory Map）
 * ============================================================ */
#define RCC_BASE        0x40021000UL
#define FLASH_BASE      0x40022000UL

/* RCC 寄存器 */
#define RCC_CR          (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR        (*(volatile uint32_t *)(RCC_BASE + 0x04))

/* FLASH 寄存器 */
#define FLASH_ACR       (*(volatile uint32_t *)(FLASH_BASE + 0x00))

/* ============================================================
 * RCC_CR 位定义
 * ============================================================ */
#define RCC_CR_HSION        (1U << 0)
#define RCC_CR_HSIRDY       (1U << 1)
#define RCC_CR_HSEON        (1U << 16)
#define RCC_CR_HSERDY       (1U << 17)
#define RCC_CR_HSEBYP       (1U << 18)
#define RCC_CR_CSSON        (1U << 19)
#define RCC_CR_PLLON        (1U << 24)
#define RCC_CR_PLLRDY       (1U << 25)

/* ============================================================
 * RCC_CFGR 位定义
 * ============================================================ */
#define RCC_CFGR_SW_MASK        (0x3U << 0)
#define RCC_CFGR_SW_HSI         (0x0U << 0)
#define RCC_CFGR_SW_HSE         (0x1U << 0)
#define RCC_CFGR_SW_PLL         (0x2U << 0)

#define RCC_CFGR_SWS_MASK       (0x3U << 2)
#define RCC_CFGR_SWS_PLL        (0x2U << 2)

#define RCC_CFGR_HPRE_DIV1      (0x0U << 4)
#define RCC_CFGR_PPRE1_DIV2     (0x4U << 8)
#define RCC_CFGR_PPRE2_DIV1     (0x0U << 11)

#define RCC_CFGR_PLLSRC_HSE     (1U << 16)
#define RCC_CFGR_PLLXTPRE_DIV1  (0U << 17)
#define RCC_CFGR_PLLMUL9        (0x7U << 18)

/* ============================================================
 * FLASH_ACR 位定义
 * ============================================================ */
#define FLASH_ACR_LATENCY_2     (0x2U << 0)
#define FLASH_ACR_PRFTBE        (1U << 4)

/* ============================================================
 * 系统变量（CMSIS 兼容约定）
 * ============================================================ */
uint32_t SystemCoreClock = 72000000;

/* ============================================================
 * SystemInit — 在 main 之前调用，配置 72MHz 时钟
 * ============================================================ */
void SystemInit(void) {
    /* === 1. 开 HSE（8MHz 外部晶振）=== */
    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY)) {
        /* 等待 HSE 稳定；如死循环说明晶振没起或没焊 */
    }

    /* === 2. Flash 等待状态 = 2（48-72MHz 必开）+ 预取缓冲 === */
    FLASH_ACR |= FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    /* === 3. 总线分频：AHB/1, APB1/2, APB2/1 === */
    RCC_CFGR = RCC_CFGR_HPRE_DIV1
             | RCC_CFGR_PPRE1_DIV2
             | RCC_CFGR_PPRE2_DIV1;

    /* === 4. 配置 PLL：HSE × 9 = 72MHz === */
    RCC_CFGR |= RCC_CFGR_PLLSRC_HSE
              | RCC_CFGR_PLLXTPRE_DIV1
              | RCC_CFGR_PLLMUL9;

    /* === 5. 开 PLL，等锁定 === */
    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) {
        /* 等待 PLL 锁定 */
    }

    /* === 6. 切 SYSCLK 到 PLL === */
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
        /* 等待切换完成 */
    }

    /* === 7. 更新系统全局变量 === */
    SystemCoreClock = 72000000;
}
