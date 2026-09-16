/**
 * nes_port.c - InfoNES HAL implementation for STM32F407VET6
 *
 * Memory layout:
 *   SRAM1 (128KB @ 0x20000000): WorkFrame (full 256x240, 120KB) + DMA buffers
 *   CCM  (64KB  @ 0x10000000): InfoNES core arrays + PRG ROM + stack
 *
 * For mapper 0 (NROM) games with PRG ≤ 16KB + CHR ≤ 8KB:
 *   CCM: RAM[8K] + SRAM[8K] + PPURAM[16K] + SPRRAM[0.25K] + PRG[16K] + CHR[8K]
 *      = 56.25KB — leaves room for stack
 *
 * Display: ILI9341 via SPI1 DMA, 256x240 centered on 320x240 (landscape mode)
 *          Use MADCTL register to rotate display (MV=1, MX=1 for landscape)
 *
 * Input: USART1 (PA9/PA10) — see uart_pad.c
 */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "main.h"
#include "ili9341.h"
#include "uart_pad.h"
#include "sd_card.h"
#include "rom_data.h"
#include "nes_port.h"
#include "../InfoNES/InfoNES.h"
#include "../InfoNES/InfoNES_System.h"

/*===================================================================*/
/*  External SPI/GPIO handles (from CubeMX-generated main.c)         */
/*===================================================================*/
extern SPI_HandleTypeDef hspi1;   /* ILI9341 display */
extern SD_HandleTypeDef  hsd;     /* microSD card (SDIO) */
extern UART_HandleTypeDef huart1; /* gamepad input */

/*===================================================================*/
/*  NES Color Palette — 64 entries, RGB565                           */
/*===================================================================*/
#define RGB565(r,g,b) ((((uint16_t)(r)>>3)<<11)|(((uint16_t)(g)>>2)<<5)|(((uint16_t)(b)>>3)))

WORD NesPalette[64] = {
    /* 0x00-0x0F */
    RGB565(0x75,0x75,0x75), RGB565(0x27,0x1B,0x8F), RGB565(0x00,0x00,0xAB),
    RGB565(0x47,0x00,0x9F), RGB565(0x8F,0x00,0x77), RGB565(0xAB,0x00,0x13),
    RGB565(0xA7,0x00,0x00), RGB565(0x7F,0x0B,0x00), RGB565(0x43,0x2F,0x00),
    RGB565(0x00,0x47,0x00), RGB565(0x00,0x51,0x00), RGB565(0x00,0x3F,0x17),
    RGB565(0x1B,0x3F,0x5F), RGB565(0x00,0x00,0x00), RGB565(0x00,0x00,0x00),
    RGB565(0x00,0x00,0x00),
    /* 0x10-0x1F */
    RGB565(0xBC,0xBC,0xBC), RGB565(0x00,0x73,0xEF), RGB565(0x23,0x3B,0xEF),
    RGB565(0x83,0x00,0xF3), RGB565(0xBF,0x00,0xBF), RGB565(0xE7,0x00,0x5B),
    RGB565(0xDB,0x2B,0x00), RGB565(0xCB,0x4F,0x0F), RGB565(0x8B,0x73,0x00),
    RGB565(0x00,0x97,0x00), RGB565(0x00,0xAB,0x00), RGB565(0x00,0x93,0x3B),
    RGB565(0x00,0x83,0x8B), RGB565(0x00,0x00,0x00), RGB565(0x00,0x00,0x00),
    RGB565(0x00,0x00,0x00),
    /* 0x20-0x2F */
    RGB565(0xFF,0xFF,0xFF), RGB565(0x3F,0xBF,0xFF), RGB565(0x5F,0x97,0xFF),
    RGB565(0xA7,0x8B,0xFD), RGB565(0xF7,0x7B,0xFF), RGB565(0xFF,0x77,0xB7),
    RGB565(0xFF,0x77,0x63), RGB565(0xFF,0x9B,0x3B), RGB565(0xF3,0xBF,0x3F),
    RGB565(0x83,0xD3,0x13), RGB565(0x4F,0xDF,0x4B), RGB565(0x58,0xF8,0x98),
    RGB565(0x00,0xEB,0xDB), RGB565(0x00,0x00,0x00), RGB565(0x00,0x00,0x00),
    RGB565(0x00,0x00,0x00),
    /* 0x30-0x3F */
    RGB565(0xFF,0xFF,0xFF), RGB565(0xAB,0xE7,0xFF), RGB565(0xC7,0xD7,0xFF),
    RGB565(0xD7,0xCB,0xFF), RGB565(0xFF,0xC7,0xFF), RGB565(0xFF,0xC7,0xDB),
    RGB565(0xFF,0xBF,0xB3), RGB565(0xFF,0xDB,0xAB), RGB565(0xFF,0xE7,0xA3),
    RGB565(0xE3,0xFF,0xA3), RGB565(0xAB,0xF3,0xBF), RGB565(0xB3,0xFF,0xCF),
    RGB565(0x9F,0xFF,0xF3), RGB565(0x00,0x00,0x00), RGB565(0x00,0x00,0x00),
    RGB565(0x00,0x00,0x00),
};

/*-------------------------------------------------------------------*/
/*  PSP stub (not used on STM32, needed to link)                     */
/*-------------------------------------------------------------------*/
int bSleep = 0;
void pgWaitV(void) { }

/*-------------------------------------------------------------------*/
/*  Frame timing state                                               */
/*-------------------------------------------------------------------*/
static uint32_t frame_start_tick = 0;

/*-------------------------------------------------------------------*/
/*  InfoNES_MemoryCopy / InfoNES_MemorySet                           */
/*-------------------------------------------------------------------*/
void *InfoNES_MemoryCopy(void *dest, const void *src, int count)
{
    return memcpy(dest, src, (size_t)count);
}

void *InfoNES_MemorySet(void *dest, int c, int count)
{
    return memset(dest, c, (size_t)count);
}

/*-------------------------------------------------------------------*/
/*  InfoNES_LoadFrame : DMA WorkFrame → ILI9341                      */
/*                                                                   */
/*  NES resolution: 256 x 240                                        */
/*  LCD resolution: 320 x 240 (landscape mode via MADCTL)            */
/*  Image is centered with 32-pixel black bars on left/right         */
/*-------------------------------------------------------------------*/
void InfoNES_LoadFrame(void)
{
    extern void HAL_GPIO_TogglePin(GPIO_TypeDef*, uint16_t);
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

    /* Wait for previous DMA chain to finish */
    while (ILI9341_DMABusy()) { UART_Pad_Poll(); }

    /* Frame pacing */
    while ((HAL_GetTick() - frame_start_tick) < 16) {
        UART_Pad_Poll();
    }
    frame_start_tick = HAL_GetTick();

    int x_off = (320 - NES_DISP_WIDTH) / 2;
    int y_off = (240 - NES_DISP_HEIGHT) / 2;

    ILI9341_SetWindow(x_off, y_off,
                      x_off + NES_DISP_WIDTH - 1,
                      y_off + NES_DISP_HEIGHT - 1);
    /* SetWindow leaves CS LOW, DC HIGH for pixel data */

    /* Kick off DMA — returns immediately. CS released by IRQ. */
    ILI9341_WriteDataDMA16((uint8_t *)WorkFrame,
                           NES_DISP_WIDTH * NES_DISP_HEIGHT * 2);
}

/*-------------------------------------------------------------------*/
/*  InfoNES_PadState : Read gamepad input                            */
/*-------------------------------------------------------------------*/
void InfoNES_PadState(DWORD *pdwPad1, DWORD *pdwPad2, DWORD *pdwSystem)
{
    /* Read key from OpenOCD memory-write */
    UART_Pad_Poll();
    uint8_t pad_state = UART_Pad_Get();

    /* Convert to InfoNES format */
    DWORD pad1 = 0;
    if (pad_state & 0x01) pad1 |= (1 << 0);  /* A */
    if (pad_state & 0x02) pad1 |= (1 << 1);  /* B */
    if (pad_state & 0x04) pad1 |= (1 << 2);  /* Select */
    if (pad_state & 0x08) pad1 |= (1 << 3);  /* Start */
    if (pad_state & 0x10) pad1 |= (1 << 4);  /* Up */
    if (pad_state & 0x20) pad1 |= (1 << 5);  /* Down */
    if (pad_state & 0x40) pad1 |= (1 << 6);  /* Left */
    if (pad_state & 0x80) pad1 |= (1 << 7);  /* Right */

    *pdwPad1 = pad1;
    *pdwPad2 = 0;      /* No player 2 in v1 */
    *pdwSystem = 0;    /* No system keys */
}

/*-------------------------------------------------------------------*/
/*  InfoNES_ReadRom : Point the emulator at the embedded ROM         */
/*                                                                   */
/*  The ROM is compiled into flash as rom_data.c (generated by       */
/*  tools/nes2c.py), so no RAM is used for it — the pointers below   */
/*  just alias into flash, which is memory-mapped and directly       */
/*  readable. See README "Adding a ROM".                             */
/*-------------------------------------------------------------------*/
int InfoNES_ReadRom(const char *pszFileName)
{
    (void)pszFileName;   /* unused — the ROM is embedded at build time */

    const unsigned char *buf = nes_rom_data;
    unsigned int         sz  = nes_rom_size;

    if (sz < sizeof(struct NesHeader_tag) ||
        buf[0] != 'N' || buf[1] != 'E' || buf[2] != 'S' || buf[3] != 0x1A) {
        printf("Embedded ROM is missing or invalid\n");
        return -1;
    }

    memcpy((void *)&NesHeader, buf, sizeof(NesHeader));

    int prg_size    = NesHeader.byRomSize  * 0x4000;  /* 16 KB units */
    int chr_size    = NesHeader.byVRomSize * 0x2000;  /*  8 KB units */
    int has_trainer = (NesHeader.byInfo1 & 0x04) ? 1 : 0;

    printf("ROM: PRG=%dKB CHR=%dKB Mapper=%d\n",
           NesHeader.byRomSize * 16,
           NesHeader.byVRomSize * 8,
           (NesHeader.byInfo2 >> 4) | (NesHeader.byInfo1 >> 4 << 4));

    const unsigned char *p = buf + sizeof(struct NesHeader_tag);
    if (has_trainer) p += 512;

    /* Alias straight into flash — nothing is copied */
    ROM = (BYTE *)p;
    p += prg_size;
    VROM = (chr_size > 0) ? (BYTE *)p : NULL;

    ROM_SRAM      = (NesHeader.byInfo1 & 0x02) ? 1 : 0;
    ROM_Mirroring = (NesHeader.byInfo1 & 0x01) ? 1 : 0;
    ROM_FourScr   = (NesHeader.byInfo1 & 0x08) ? 1 : 0;

    printf("ROM ready (%u bytes in flash)\n", sz);
    return 0;
}

/*-------------------------------------------------------------------*/
/*  InfoNES_ReleaseRom                                               */
/*-------------------------------------------------------------------*/
void InfoNES_ReleaseRom(void)
{
    /* ROM/VROM alias into flash — nothing to free */
    ROM  = NULL;
    VROM = NULL;
}

/*-------------------------------------------------------------------*/
/*  InfoNES_Wait : Frame timing                                      */
/*-------------------------------------------------------------------*/
void InfoNES_Wait(void)
{
    /* Called 262 times/frame (per scanline). Must be instant.
     * Frame pacing belongs in InfoNES_LoadFrame(). */
    UART_Pad_Poll();
}

/*-------------------------------------------------------------------*/
/*  InfoNES_DebugPrint / InfoNES_MessageBox                          */
/*-------------------------------------------------------------------*/
void InfoNES_DebugPrint(char *pszMsg)
{
    /* Output to UART for debug, or ignore in release */
    printf("%s\n", pszMsg);
}

void InfoNES_MessageBox(char *pszMsg, ...)
{
    va_list args;
    va_start(args, pszMsg);
    printf("[NES] ");
    vprintf(pszMsg, args);
    printf("\n");
    va_end(args);
}

/*-------------------------------------------------------------------*/
/*  InfoNES_Menu : Load the ROM                                      */
/*                                                                   */
/*  The ROM is embedded at build time, so there is no selection to   */
/*  make. InfoNES_Load() reads the ROM *and* resets the CPU/PPU —    */
/*  calling InfoNES_ReadRom() alone would leave the mapper          */
/*  uninitialised and the emulator would never start.               */
/*-------------------------------------------------------------------*/
int InfoNES_Menu(void)
{
    return InfoNES_Load(NULL);
}

/*-------------------------------------------------------------------*/
/*  Audio stubs (not implemented in v1)                              */
/*-------------------------------------------------------------------*/
void InfoNES_SoundInit(void) { }
int  InfoNES_SoundOpen(int samples_per_sync, int sample_rate) { return 0; }
void InfoNES_SoundClose(void) { }
void InfoNES_SoundOutput(int samples, BYTE *w1, BYTE *w2, BYTE *w3,
                          BYTE *w4, BYTE *w5) { }

/*===================================================================*/
/*  Platform helpers (called from main.c or interrupt handlers)      */
/*===================================================================*/

static uint32_t g_tick_ms = 0;

void NES_DisplayInit(void)
{
    ILI9341_Init();
    ILI9341_SetRotation(LANDSCAPE);  /* Use landscape mode for NES */
    ILI9341_FillScreen(ILI9341_BLACK);
}

void NES_PadPoll(void)
{
    UART_Pad_Poll();
}

uint8_t NES_PadGet(void)
{
    return UART_Pad_Get();
}

uint32_t NES_GetTick(void)
{
    return g_tick_ms;
}

/* Called from SysTick_Handler (1ms) */
void NES_TickInc(void)
{
    g_tick_ms++;
}

/* printf now goes to semihosting (OpenOCD console) via rdimon.specs */
