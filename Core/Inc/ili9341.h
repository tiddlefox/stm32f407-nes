/**
 * ili9341.h - ILI9341 TFT LCD driver adapter for STM32F407
 *
 * Wraps the martnak/STM32-ILI9341 HAL driver with NES-specific helpers.
 *
 * Pin mapping (SPI1):
 *   PA5  = SCK
 *   PA7  = MOSI (SDI)
 *   PA6  = MISO (optional)
 *   PB6  = CS   (software GPIO)
 *   PB7  = RST  (hardware reset)
 *   PB8  = DC   (data/command select)
 *   PB9  = BL   (backlight PWM, TIM4_CH4)
 */

#ifndef ILI9341_H
#define ILI9341_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------*/
/*  Color constants (RGB565)                                         */
/*-------------------------------------------------------------------*/
#define ILI9341_BLACK       0x0000
#define ILI9341_WHITE       0xFFFF
#define ILI9341_RED         0xF800
#define ILI9341_GREEN       0x07E0
#define ILI9341_BLUE        0x001F
#define ILI9341_GRAY        0x8410
#define ILI9341_DARKGRAY    0x4208

/*-------------------------------------------------------------------*/
/*  Rotation modes                                                   */
/*-------------------------------------------------------------------*/
#define PORTRAIT        0
#define LANDSCAPE       1
#define PORTRAIT_FLIP   2
#define LANDSCAPE_FLIP  3

/*-------------------------------------------------------------------*/
/*  Display dimensions                                               */
/*-------------------------------------------------------------------*/
#define ILI9341_WIDTH   320
#define ILI9341_HEIGHT  240

/*-------------------------------------------------------------------*/
/*  Initialization                                                   */
/*-------------------------------------------------------------------*/

/**
 * Initialize ILI9341 display:
 *   - Configure SPI1 for 21MHz, 8-bit, CPOL=0, CPHA=1
 *   - Send init command sequence (exit sleep, set pixel format, etc.)
 *   - Turn on backlight via PWM on PB9
 */
void ILI9341_Init(void);

/*-------------------------------------------------------------------*/
/*  Drawing primitives                                               */
/*-------------------------------------------------------------------*/

/**
 * Set the active drawing window. Subsequent data writes to GRAM
 * will fill this rectangular region.
 */
void ILI9341_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * Write pixel data via blocking SPI transmit.
 * Call ILI9341_SetWindow first.
 */
void ILI9341_WriteData(const uint8_t *data, uint32_t len);

/**
 * Write pixel data via SPI DMA (non-blocking).
 * Caller must wait for DMA completion before calling again.
 */
void ILI9341_WriteDataDMA(const uint8_t *data, uint32_t len);
void ILI9341_DMA_Init(void);
void ILI9341_WriteDataDMA16(const uint8_t *data, uint32_t byte_len);
uint8_t ILI9341_DMABusy(void);
void ILI9341_DMAComplete(void);

/**
 * Fill entire screen with a single color (RGB565).
 */
void ILI9341_FillScreen(uint16_t color);

/**
 * Fill a rectangular region with a single color.
 */
void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Set rotation (landscape/portrait).
 * Uses MADCTL register (0x36) with MV, MX, MY, BGR bits.
 */
void ILI9341_SetRotation(uint8_t rotation);

/**
 * Set backlight brightness (0-100%).
 */
void ILI9341_SetBacklight(uint8_t percent);

/**
 * Draw a single pixel.
 */
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/*-------------------------------------------------------------------*/
/*  Font / Text (for ROM menu)                                       */
/*-------------------------------------------------------------------*/

/**
 * Draw a single character at (x, y) in 5x7 font.
 */
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);

/**
 * Draw a null-terminated string at (x, y).
 */
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);

#ifdef __cplusplus
}
#endif

#endif /* ILI9341_H */
