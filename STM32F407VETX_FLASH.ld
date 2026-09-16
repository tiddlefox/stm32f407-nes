/*
 * Linker script for STM32F407VET6
 * Flash: 512KB, SRAM1: 128KB, CCM: 64KB
 *
 * Memory layout:
 *   FLASH:  code + rodata (512KB @ 0x08000000)
 *   SRAM1:  WorkFrame (120KB) + data + bss + heap + DMA buffers (128KB)
 *   CCM:    InfoNES core arrays + ROM + stack (64KB, no DMA)
 */

ENTRY(Reset_Handler)

_estack = ORIGIN(SRAM1) + LENGTH(SRAM1);  /* Stack top = end of SRAM1 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    SRAM1 (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    CCM   (rwx) : ORIGIN = 0x10000000, LENGTH = 64K
}

SECTIONS
{
    /* Vector table and code in Flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
    } >FLASH

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >FLASH

    .preinit_array     :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH

    _sidata = LOADADDR(.data);

    /* Data in SRAM1 (includes WorkFrame for DMA) */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >SRAM1 AT> FLASH

    /* BSS in SRAM1 */
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
    } >SRAM1

    /* CCM RAM for InfoNES core data (no DMA) */
    .ccm (NOLOAD) :
    {
        . = ALIGN(4);
        _sccm = .;
        *(.ccm)
        *(.ccm*)
        . = ALIGN(4);
        _eccm = .;
    } >CCM

    /* Remove unused allocations from standard lib */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }

    /* Heap marker for _sbrk */
    end = .;
    _end = .;
    PROVIDE(end = .);
}
