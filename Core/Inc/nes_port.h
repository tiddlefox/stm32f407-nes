/**
 * nes_port.h - InfoNES HAL declarations for STM32F407
 *
 * Platform adapters that InfoNES requires from the host system:
 * - Display framebuffer output (ILI9341 via SPI1 DMA)
 * - Input reading (ESP-01S UART -> PAD1_Latch)
 * - ROM loading from SD card (FatFs)
 * - Memory helpers (memcpy/memset wrappers)
 * - Timing (SysTick-based frame sync)
 */

#ifndef NES_PORT_H
#define NES_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*  Display helpers                                                   */
/*-------------------------------------------------------------------*/

/**
 * Initialize ILI9341 and prepare for frame rendering.
 * Called once at startup from main().
 */
void NES_DisplayInit(void);

/**
 * Send one scanline of pixels to ILI9341 at position (x, line_y).
 * Used for line-by-line rendering to avoid storing the full 120KB
 * frame buffer. Each pixel is 16-bit RGB565.
 *
 * @param line_y   scanline Y position (0..239)
 * @param pixels   pointer to 256 * 2 bytes of RGB565 data
 * @param width    pixels per line (256 for NES)
 */
void NES_DisplayLine(uint16_t line_y, const uint16_t *pixels, uint16_t width);

/**
 * Signal end of frame. Flushes any pending DMA transfers and
 * prepares for next frame.
 */
void NES_DisplayEndFrame(void);

/*-------------------------------------------------------------------*/
/*  Input helpers                                                     */
/*-------------------------------------------------------------------*/

/**
 * Poll ESP-01S for pending gamepad data.
 * Parses any received HTTP key events and updates the internal
 * pad state buffer.
 */
void NES_PadPoll(void);

/**
 * Returns the current pad state (set by NES_PadPoll).
 * Called by InfoNES_PadState().
 *
 * @return 8-bit pad state (NES standard bit layout)
 */
uint8_t NES_PadGet(void);

/*-------------------------------------------------------------------*/
/*  Storage helpers                                                   */
/*-------------------------------------------------------------------*/

/**
 * Initialize SD card (SPI2) and mount FatFs filesystem.
 * @return 0 on success, non-zero on failure
 */
int NES_SDInit(void);

/**
 * List .nes files on SD card to a buffer.
 * Each entry is a null-terminated filename, list ends with empty string.
 * @return number of ROMs found
 */
int NES_SDListRoms(char *list_buf, int buf_size);

/**
 * Read an entire .nes file from SD card into a buffer.
 * @param fname   filename on SD card
 * @param buf     output buffer (caller must free)
 * @param size    output: file size
 * @return 0 on success, non-zero on failure
 */
int NES_SDReadFile(const char *fname, uint8_t **buf, uint32_t *size);

/*-------------------------------------------------------------------*/
/*  Timing helpers                                                    */
/*-------------------------------------------------------------------*/

/**
 * Wait until the next frame boundary (NTSC: ~16.67ms per frame).
 * Uses SysTick or a hardware timer interrupt to synchronize.
 */
void NES_FrameSync(void);

/*-------------------------------------------------------------------*/
/*  Utility                                                          */
/*-------------------------------------------------------------------*/

uint32_t NES_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* NES_PORT_H */
