/**
 * ili9341.c - ILI9341 TFT LCD driver implementation for STM32F407
 *
 * Based on martnak/STM32-ILI9341, adapted for STM32F407VET6.
 * SPI1: PA5(SCK), PA7(MOSI), PB6(CS), PB7(RST), PB8(DC), PB9(BL)
 */

#include <string.h>
#include "main.h"
#include "ili9341.h"

/*-------------------------------------------------------------------*/
/*  External SPI handle & GPIO pin definitions                       */
/*-------------------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;

/* GPIO port/pin definitions for control signals */
#define LCD_CS_PORT   GPIOB
#define LCD_CS_PIN    GPIO_PIN_6
#define LCD_RST_PORT  GPIOB
#define LCD_RST_PIN   GPIO_PIN_7
#define LCD_DC_PORT   GPIOB
#define LCD_DC_PIN    GPIO_PIN_8
#define LCD_BL_PORT   GPIOB
#define LCD_BL_PIN    GPIO_PIN_9

/*-------------------------------------------------------------------*/
/*  Macros for control signals                                       */
/*-------------------------------------------------------------------*/
#define CS_LOW()    HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET)
#define CS_HIGH()   HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET)
#define DC_LOW()    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET) /* Command */
#define DC_HIGH()   HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET)   /* Data */
#define RST_LOW()   HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET)
#define RST_HIGH()  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET)
#define BL_ON()     HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_SET)
#define BL_OFF()    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, GPIO_PIN_RESET)

/*-------------------------------------------------------------------*/
/*  Static state                                                     */
/*-------------------------------------------------------------------*/
static uint16_t disp_width  = 320;
static uint16_t disp_height = 240;
static uint8_t  first_frame = 1;  /* 0x2C first frame, 0x3C after */
DMA_HandleTypeDef hdma_spi1_tx;

/*-------------------------------------------------------------------*/
/*  Low-level SPI helpers                                            */
/*-------------------------------------------------------------------*/

static void SPI_WriteCmd(uint8_t cmd)
{
    CS_LOW();
    DC_LOW();  /* Command mode */
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    CS_HIGH();
}

static void SPI_WriteData8(uint8_t data)
{
    CS_LOW();
    DC_HIGH(); /* Data mode */
    HAL_SPI_Transmit(&hspi1, &data, 1, 10);
    CS_HIGH();
}

static void SPI_WriteDataMulti(const uint8_t *data, uint32_t len)
{
    CS_LOW();
    DC_HIGH();
    HAL_SPI_Transmit(&hspi1, (uint8_t *)data, (uint16_t)len, HAL_MAX_DELAY);
    CS_HIGH();
}

/*-------------------------------------------------------------------*/
/*  ILI9341_Init : Full initialization sequence                      */
/*-------------------------------------------------------------------*/

void ILI9341_Init(void)
{
    /* Hardware reset */
    RST_HIGH();
    HAL_Delay(5);
    RST_LOW();
    HAL_Delay(20);
    RST_HIGH();
    HAL_Delay(150);

    /* Software reset */
    SPI_WriteCmd(0x01); /* SW reset */
    HAL_Delay(150);

    /* Exit sleep */
    SPI_WriteCmd(0x11); /* Sleep out */
    HAL_Delay(150);

    /* Pixel format: 16-bit (RGB565) */
    SPI_WriteCmd(0x3A);
    SPI_WriteData8(0x55); /* 16 bits/pixel */
    HAL_Delay(10);

    /* Set rotation: landscape (MV=1, MX=1) */
    ILI9341_SetRotation(LANDSCAPE);

    /* Normal display mode on */
    SPI_WriteCmd(0x13); /* Normal display on */

    /* Display on */
    SPI_WriteCmd(0x29); /* Display on */
    HAL_Delay(100);

    /* Backlight on */
    BL_ON();
}

/*-------------------------------------------------------------------*/
/*  ILI9341_SetWindow : Set column/page address                      */
/*-------------------------------------------------------------------*/

void ILI9341_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t cmd, buf[4];

    /* All commands in one CS transaction */
    CS_LOW();

    /* CASET: 0x2A + 4 bytes */
    DC_LOW(); cmd = 0x2A; HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    buf[0]=(x1>>8)&0xFF; buf[1]=x1&0xFF; buf[2]=(x2>>8)&0xFF; buf[3]=x2&0xFF;
    HAL_SPI_Transmit(&hspi1, buf, 4, 10);

    /* PASET: 0x2B + 4 bytes */
    DC_LOW(); cmd = 0x2B; HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    buf[0]=(y1>>8)&0xFF; buf[1]=y1&0xFF; buf[2]=(y2>>8)&0xFF; buf[3]=y2&0xFF;
    HAL_SPI_Transmit(&hspi1, buf, 4, 10);

    /* 0x2C: Memory Write (reset address counter to window start) */
    DC_LOW();
    cmd = 0x2C;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    /* CS stays LOW for pixel data DMA that follows */
}

/* DMA init for SPI1 TX */
void ILI9341_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    hdma_spi1_tx.Instance = DMA2_Stream3;
    hdma_spi1_tx.Init.Channel = DMA_CHANNEL_3;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_spi1_tx);
    __HAL_LINKDMA(&hspi1, hdmatx, hdma_spi1_tx);
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

/* Chained DMA state */
static const uint8_t *dma_data_ptr;
static uint32_t       dma_remain;

/* Non-blocking 8-bit DMA send — kick off first chunk, IRQ chains rest */
void ILI9341_WriteDataDMA16(const uint8_t *data, uint32_t byte_len)
{
    /* Clear RX junk */
    while (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE))
        (void)hspi1.Instance->DR;
    if (__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_OVR)) {
        (void)hspi1.Instance->DR;
        __HAL_SPI_CLEAR_OVRFLAG(&hspi1);
    }

    dma_data_ptr = data;
    dma_remain = byte_len;

    /* Kick off first chunk (max 65535 bytes per DMA) */
    uint16_t chunk = (byte_len > 65535) ? 65535 : (uint16_t)byte_len;
    dma_data_ptr += chunk;
    dma_remain -= chunk;
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)data, chunk);
}

uint8_t ILI9341_DMABusy(void)
{
    return (hdma_spi1_tx.Instance->CR & DMA_SxCR_EN) ? 1 : 0;
}

/* DMA IRQ handler */
void DMA2_Stream3_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_spi1_tx); }

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance != SPI1) return;

    if (dma_remain > 0) {
        /* Chain next chunk */
        uint16_t chunk = (dma_remain > 65535) ? 65535 : (uint16_t)dma_remain;
        HAL_SPI_Transmit_DMA(hspi, (uint8_t *)dma_data_ptr, chunk);
        dma_data_ptr += chunk;
        dma_remain -= chunk;
    } else {
        /* All done — release CS */
        CS_HIGH();
    }
}

/*-------------------------------------------------------------------*/
/*  ILI9341_WriteData : Blocking write                               */
/*-------------------------------------------------------------------*/

void ILI9341_WriteData(const uint8_t *data, uint32_t len)
{
    CS_LOW();
    DC_HIGH(); /* Data mode */

    /* Split into chunks if needed (HAL max 65535 per transfer) */
    while (len > 0) {
        uint16_t chunk = (len > 65535) ? 65535 : (uint16_t)len;
        HAL_SPI_Transmit(&hspi1, (uint8_t *)data, chunk, HAL_MAX_DELAY);
        data += chunk;
        len -= chunk;
    }

    CS_HIGH();
}

/*-------------------------------------------------------------------*/
/*  ILI9341_WriteDataDMA : Non-blocking DMA write                    */
/*-------------------------------------------------------------------*/

void ILI9341_WriteDataDMA(const uint8_t *data, uint32_t len)
{
    CS_LOW();
    DC_HIGH();
    HAL_SPI_Transmit_DMA(&hspi1, (uint8_t *)data, (uint16_t)len);
    /* CS will be released by caller after DMA complete check */
}

/*-------------------------------------------------------------------*/
/*  ILI9341_FillScreen : Fill entire display with color              */
/*-------------------------------------------------------------------*/

void ILI9341_FillScreen(uint16_t color)
{
    ILI9341_FillRect(0, 0, disp_width, disp_height, color);
}

void ILI9341_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ILI9341_SetWindow(x, y, x + w - 1, y + h - 1);

    uint32_t pixels = (uint32_t)w * h;
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;

    CS_LOW();
    DC_HIGH();

    /* Send same color pair repeatedly */
    for (uint32_t i = 0; i < pixels; i++) {
        uint8_t pair[2] = { hi, lo };
        HAL_SPI_Transmit(&hspi1, pair, 2, HAL_MAX_DELAY);
    }

    CS_HIGH();
}

/*-------------------------------------------------------------------*/
/*  ILI9341_SetRotation : Set display orientation via MADCTL         */
/*-------------------------------------------------------------------*/

void ILI9341_SetRotation(uint8_t rotation)
{
    SPI_WriteCmd(0x36); /* MADCTL: Memory Access Control */

    /* MADCTL bit layout:
     *   bit7: MY (row address order)   0=top→bottom, 1=bottom→top
     *   bit6: MX (column address order) 0=left→right, 1=right→left
     *   bit5: MV (row/column exchange)  0=normal, 1=swap
     *   bit4: ML (vertical refresh)    0=top→bottom
     *   bit3: BGR (RGB/BGR order)      0=RGB, 1=BGR
     *   bit2: MH (horizontal refresh)   0=left→right
     */
    switch (rotation) {
        case PORTRAIT:
            SPI_WriteData8(0x08); /* BGR=1 */
            disp_width = 240;
            disp_height = 320;
            break;
        case LANDSCAPE:
            SPI_WriteData8(0x28); /* MV=1, BGR=1 — landscape 320x240 */
            disp_width = 320;
            disp_height = 240;
            break;
        case PORTRAIT_FLIP:
            SPI_WriteData8(0x88); /* MY=1, BGR=1 */
            disp_width = 240;
            disp_height = 320;
            break;
        case LANDSCAPE_FLIP:
            SPI_WriteData8(0xE8); /* MY=1, MX=1, MV=1, BGR=1 */
            disp_width = 320;
            disp_height = 240;
            break;
        default:
            SPI_WriteData8(0x28);
            disp_width = 320;
            disp_height = 240;
            break;
    }
}

/*-------------------------------------------------------------------*/
/*  Backlight control                                                */
/*-------------------------------------------------------------------*/

void ILI9341_SetBacklight(uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent == 0) {
        BL_OFF();
    } else {
        BL_ON();
        /* PWM duty cycle control via TIM4_CH4 would go here */
        /* For simple on/off, we just use GPIO */
    }
}

/*-------------------------------------------------------------------*/
/*  ILI9341_DrawPixel                                                */
/*-------------------------------------------------------------------*/

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    ILI9341_SetWindow(x, y, x, y);
    uint8_t buf[2] = { (color >> 8) & 0xFF, color & 0xFF };
    SPI_WriteDataMulti(buf, 2);
}

/*-------------------------------------------------------------------*/
/*  5x7 Font data (ASCII 0x20-0x7F)                                  */
/*-------------------------------------------------------------------*/
static const uint8_t Font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /*  32 SPACE */
    {0x00,0x00,0x5F,0x00,0x00}, /*  33 ! */
    {0x00,0x07,0x00,0x07,0x00}, /*  34 " */
    {0x14,0x7F,0x14,0x7F,0x14}, /*  35 # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /*  36 $ */
    {0x23,0x13,0x08,0x64,0x62}, /*  37 % */
    {0x36,0x49,0x55,0x22,0x50}, /*  38 & */
    {0x00,0x05,0x03,0x00,0x00}, /*  39 ' */
    {0x00,0x1C,0x22,0x41,0x00}, /*  40 ( */
    {0x00,0x41,0x22,0x1C,0x00}, /*  41 ) */
    {0x08,0x2A,0x1C,0x2A,0x08}, /*  42 * */
    {0x08,0x08,0x3E,0x08,0x08}, /*  43 + */
    {0x00,0x50,0x30,0x00,0x00}, /*  44 , */
    {0x08,0x08,0x08,0x08,0x08}, /*  45 - */
    {0x00,0x60,0x60,0x00,0x00}, /*  46 . */
    {0x20,0x10,0x08,0x04,0x02}, /*  47 / */
    {0x3E,0x51,0x49,0x45,0x3E}, /*  48 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /*  49 1 */
    {0x42,0x61,0x51,0x49,0x46}, /*  50 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /*  51 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /*  52 4 */
    {0x27,0x45,0x45,0x45,0x39}, /*  53 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /*  54 6 */
    {0x01,0x71,0x09,0x05,0x03}, /*  55 7 */
    {0x36,0x49,0x49,0x49,0x36}, /*  56 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /*  57 9 */
    {0x00,0x36,0x36,0x00,0x00}, /*  58 : */
    {0x00,0x56,0x36,0x00,0x00}, /*  59 ; */
    {0x00,0x08,0x14,0x22,0x41}, /*  60 < */
    {0x14,0x14,0x14,0x14,0x14}, /*  61 = */
    {0x41,0x22,0x14,0x08,0x00}, /*  62 > */
    {0x02,0x01,0x51,0x09,0x06}, /*  63 ? */
    {0x32,0x49,0x79,0x41,0x3E}, /*  64 @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /*  65 A */
    {0x7F,0x49,0x49,0x49,0x36}, /*  66 B */
    {0x3E,0x41,0x41,0x41,0x22}, /*  67 C */
    {0x7F,0x41,0x41,0x22,0x1C}, /*  68 D */
    {0x7F,0x49,0x49,0x49,0x41}, /*  69 E */
    {0x7F,0x09,0x09,0x01,0x01}, /*  70 F */
    {0x3E,0x41,0x41,0x51,0x32}, /*  71 G */
    {0x7F,0x08,0x08,0x08,0x7F}, /*  72 H */
    {0x00,0x41,0x7F,0x41,0x00}, /*  73 I */
    {0x20,0x40,0x41,0x3F,0x01}, /*  74 J */
    {0x7F,0x08,0x14,0x22,0x41}, /*  75 K */
    {0x7F,0x40,0x40,0x40,0x40}, /*  76 L */
    {0x7F,0x02,0x04,0x02,0x7F}, /*  77 M */
    {0x7F,0x04,0x08,0x10,0x7F}, /*  78 N */
    {0x3E,0x41,0x41,0x41,0x3E}, /*  79 O */
    {0x7F,0x09,0x09,0x09,0x06}, /*  80 P */
    {0x3E,0x41,0x51,0x21,0x5E}, /*  81 Q */
    {0x7F,0x09,0x19,0x29,0x46}, /*  82 R */
    {0x46,0x49,0x49,0x49,0x31}, /*  83 S */
    {0x01,0x01,0x7F,0x01,0x01}, /*  84 T */
    {0x3F,0x40,0x40,0x40,0x3F}, /*  85 U */
    {0x1F,0x20,0x40,0x20,0x1F}, /*  86 V */
    {0x7F,0x20,0x18,0x20,0x7F}, /*  87 W */
    {0x63,0x14,0x08,0x14,0x63}, /*  88 X */
    {0x03,0x04,0x78,0x04,0x03}, /*  89 Y */
    {0x61,0x51,0x49,0x45,0x43}, /*  90 Z */
    {0x00,0x00,0x7F,0x41,0x41}, /*  91 [ */
    {0x02,0x04,0x08,0x10,0x20}, /*  92 backslash */
    {0x41,0x41,0x7F,0x00,0x00}, /*  93 ] */
    {0x04,0x02,0x01,0x02,0x04}, /*  94 ^ */
    {0x40,0x40,0x40,0x40,0x40}, /*  95 _ */
    {0x00,0x01,0x02,0x04,0x00}, /*  96 ` */
    {0x20,0x54,0x54,0x54,0x78}, /*  97 a */
    {0x7F,0x48,0x44,0x44,0x38}, /*  98 b */
    {0x38,0x44,0x44,0x44,0x20}, /*  99 c */
    {0x38,0x44,0x44,0x48,0x7F}, /* 100 d */
    {0x38,0x54,0x54,0x54,0x18}, /* 101 e */
    {0x08,0x7E,0x09,0x01,0x02}, /* 102 f */
    {0x08,0x14,0x54,0x54,0x3C}, /* 103 g */
    {0x7F,0x08,0x04,0x04,0x78}, /* 104 h */
    {0x00,0x44,0x7D,0x40,0x00}, /* 105 i */
    {0x20,0x40,0x44,0x3D,0x00}, /* 106 j */
    {0x00,0x7F,0x10,0x28,0x44}, /* 107 k */
    {0x00,0x41,0x7F,0x40,0x00}, /* 108 l */
    {0x7C,0x04,0x18,0x04,0x78}, /* 109 m */
    {0x7C,0x08,0x04,0x04,0x78}, /* 110 n */
    {0x38,0x44,0x44,0x44,0x38}, /* 111 o */
    {0x7C,0x14,0x14,0x14,0x08}, /* 112 p */
    {0x08,0x14,0x14,0x18,0x7C}, /* 113 q */
    {0x7C,0x08,0x04,0x04,0x08}, /* 114 r */
    {0x48,0x54,0x54,0x54,0x20}, /* 115 s */
    {0x04,0x3F,0x44,0x40,0x20}, /* 116 t */
    {0x3C,0x40,0x40,0x20,0x7C}, /* 117 u */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 118 v */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 119 w */
    {0x44,0x28,0x10,0x28,0x44}, /* 120 x */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 121 y */
    {0x44,0x64,0x54,0x4C,0x44}, /* 122 z */
    {0x00,0x08,0x36,0x41,0x00}, /* 123 { */
    {0x00,0x00,0x7F,0x00,0x00}, /* 124 | */
    {0x00,0x41,0x36,0x08,0x00}, /* 125 } */
    {0x08,0x08,0x2A,0x1C,0x08}, /* 126 ~ */
};

/*-------------------------------------------------------------------*/
/*  ILI9341_DrawChar : Draw 5x7 font character                       */
/*-------------------------------------------------------------------*/

void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg)
{
    if (c < 0x20 || c > 0x7E) c = ' ';
    int idx = c - 0x20;

    /* Window: 6px wide × 8px tall = 48 pixels total */
    ILI9341_SetWindow(x, y, x + 5, y + 7);

    CS_LOW();
    DC_HIGH();

    for (int row = 0; row < 8; row++) {
        uint8_t line = (row < 7 && idx < 95) ? Font5x7[idx][row] : 0x00;
        for (int col = 0; col < 6; col++) {
            uint16_t px;
            if (col < 5 && row < 7) {
                px = (line & (1 << (4 - col))) ? color : bg;
            } else {
                px = bg; /* gap column / gap row = background */
            }
            uint8_t buf[2] = { (px >> 8) & 0xFF, px & 0xFF };
            HAL_SPI_Transmit(&hspi1, buf, 2, HAL_MAX_DELAY);
        }
    }

    CS_HIGH();
}

/*-------------------------------------------------------------------*/
/*  ILI9341_DrawString : Draw a string                               */
/*-------------------------------------------------------------------*/

/* Diagnosis: raw fill entire screen with one color, bypass all wrappers */
void ILI9341_DiagFill(uint16_t color) {
    uint8_t cmd, buf[4];
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;

    /* CASET: 0 .. 239 */
    CS_LOW(); DC_LOW();
    cmd = 0x2A; HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    buf[0]=0; buf[1]=0; buf[2]=0; buf[3]=239;
    HAL_SPI_Transmit(&hspi1, buf, 4, 10);
    CS_HIGH(); HAL_Delay(1);

    /* PASET: 0 .. 319 */
    CS_LOW(); DC_LOW();
    cmd = 0x2B; HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    buf[0]=0; buf[1]=0; buf[2]=1; buf[3]=63; /* 0x013F = 319 */
    HAL_SPI_Transmit(&hspi1, buf, 4, 10);
    CS_HIGH(); HAL_Delay(1);

    /* RAMWR + pixel data in one CS transaction */
    CS_LOW(); DC_LOW();
    cmd = 0x2C; HAL_SPI_Transmit(&hspi1, &cmd, 1, 10);
    DC_HIGH();
    for (int i = 0; i < 240*320; i++) {
        uint8_t p[2] = {hi, lo};
        HAL_SPI_Transmit(&hspi1, p, 2, HAL_MAX_DELAY);
    }
    CS_HIGH();
}

void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str,
                         uint16_t color, uint16_t bg)
{
    while (*str) {
        ILI9341_DrawChar(x, y, *str, color, bg);
        x += 6; /* 5 pixel char + 1 pixel gap */
        if (x > disp_width - 6) {
            x = 0;
            y += 8;
        }
        str++;
    }
}
