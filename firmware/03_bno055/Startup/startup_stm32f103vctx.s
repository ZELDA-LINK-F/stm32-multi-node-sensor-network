/*
 * 启动文件 — 最小版，仅支持 Reset / NMI / HardFault
 * B.3 FreeRTOS 时补全中断向量表 + SystemInit() 配 72MHz 时钟
 *
 * 流程：
 *   1. 设置栈指针（硬件自动做，前 4 字节是 _estack）
 *   2. Reset_Handler：把 .data 从 Flash 拷到 RAM，清零 .bss
 *   3. 调用 main()
 */

    .syntax unified
    .cpu cortex-m3
    .fpu softvfp
    .thumb

.global g_pfnVectors
.global Default_Handler

/* ============================================================
 * 中断向量表（STM32F103 有 16 系统异常 + 60 外设中断）
 * ============================================================ */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object

g_pfnVectors:
    .word _estack              /* 0x00 初始栈指针（MSP） */
    .word Reset_Handler        /* 0x04 Reset */
    .word NMI_Handler          /* 0x08 NMI */
    .word HardFault_Handler    /* 0x0C HardFault */
    .word 0                    /* 0x10 MemManage */
    .word 0                    /* 0x14 BusFault */
    .word 0                    /* 0x18 UsageFault */
    .word 0                    /* 0x1C reserved */
    .word 0                    /* 0x20 reserved */
    .word 0                    /* 0x24 reserved */
    .word 0                    /* 0x28 reserved */
    .word 0                    /* 0x2C SVCall */
    .word 0                    /* 0x30 DebugMon */
    .word 0                    /* 0x34 reserved */
    .word 0                    /* 0x38 PendSV */
    .word 0                    /* 0x3C SysTick */
    /* 外设中断 0x40~0x100，B.3 FreeRTOS 移植时按需补全 */
    .set vector_count, 60
    .rept vector_count
    .word 0
    .endr

    .size g_pfnVectors, .-g_pfnVectors

/* ============================================================
 * Reset_Handler
 * ============================================================ */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function

Reset_Handler:
    /* 1. 把 .data 从 Flash 拷贝到 RAM */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4

LoopCopyDataInit:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   CopyDataInit

    /* 2. 清零 .bss */
    ldr   r2, =_sbss
    ldr   r4, =_ebss
    movs  r3, #0
    b     LoopFillZeroBss

FillZeroBss:
    str   r3, [r2]
    adds  r2, r2, #4

LoopFillZeroBss:
    cmp   r2, r4
    bcc   FillZeroBss

    /* 3. B.1 简化：直接调 main（跳过 SystemInit，B.3 FreeRTOS 时配 72MHz） */
    bl    main

LoopForever:
    b     LoopForever

    .size Reset_Handler, .-Reset_Handler

/* ============================================================
 * 默认异常处理
 * ============================================================ */
    .section .text.Default_Handler,"ax",%progbits
    .weak Default_Handler
    .type Default_Handler, %function

Default_Handler:
Infinite_Loop:
    b   Infinite_Loop
    .size Default_Handler, .-Default_Handler

    .weak NMI_Handler
    .thumb_set NMI_Handler,Default_Handler

    .weak HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler

    .end
