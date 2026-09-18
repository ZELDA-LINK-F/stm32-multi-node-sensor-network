/*
 * STM32F103VET6 链接脚本
 * Flash: 0x08000000, 512KB
 * RAM:   0x20000000, 64KB
 *
 * 注意：gcc 启动文件里的符号（_sidata 等）依赖这个脚本定义
 */

ENTRY(Reset_Handler)

/* 最高地址 = RAM 结束 = 栈顶（栈向下增长） */
_estack = 0x20010000;    /* 64KB RAM 结束地址 */

MEMORY
{
    FLASH (rx)   : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (xrw)  : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    /* 中断向量表，必须放在 Flash 起始位置 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* 代码段 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* ARM 库需要的额外段 */
    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } >FLASH

    /* 已初始化数据段（拷贝到 RAM） */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* 未初始化数据段（清零） */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* 用户堆栈（保留空间） */
    ._user_heap_stack :
    {
        . = ALIGN(4);
        . = . + 0x1000;     /* 4KB heap+stack */
    } >RAM

    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
