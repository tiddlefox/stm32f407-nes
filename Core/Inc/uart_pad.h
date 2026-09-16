/**
 * uart_pad.h - Gamepad input over UART
 *
 * Receives single-byte key codes from a PC over USART1 (PA9/PA10).
 * See tools/pad_serial.py for the matching PC-side sender.
 *
 * Key codes (single ASCII char):
 *   WASD = D-Pad       J = B       K = A
 *   Enter = Start      Space = Select      0 = Release all
 *
 * NOTE: Not yet verified on physical hardware.
 */

#ifndef UART_PAD_H
#define UART_PAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pad state bit layout (NES standard) */
#define PAD_A      0x01
#define PAD_B      0x02
#define PAD_SELECT 0x04
#define PAD_START  0x08
#define PAD_UP     0x10
#define PAD_DOWN   0x20
#define PAD_LEFT   0x40
#define PAD_RIGHT  0x80

/* Start interrupt-driven reception. Requires USART1 to be initialised. */
void UART_Pad_Init(void);

/* Translate the last received byte into pad state. Call once per frame. */
void UART_Pad_Poll(void);

/* Current pad state (PAD_* bit mask). */
uint8_t UART_Pad_Get(void);

/* Called from the UART RX interrupt to hand over a received byte. */
void UART_Pad_RxCallback(uint8_t byte);

#ifdef __cplusplus
}
#endif

#endif /* UART_PAD_H */
