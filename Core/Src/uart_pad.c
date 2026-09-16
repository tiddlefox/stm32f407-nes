/**
 * uart_pad.c - Gamepad input over UART
 *
 * Receives single ASCII characters from a PC (via USB-TTL adapter or any
 * UART bridge) and converts them to NES pad state.
 *
 * Wiring:
 *   PC/USB-TTL TX  ->  PA10 (USART1_RX)
 *   PC/USB-TTL RX  ->  PA9  (USART1_TX)   [optional, for debug output]
 *   GND            ->  GND                [required]
 *
 * Key map:
 *   W / Up arrow    = D-Pad Up
 *   S / Down arrow  = D-Pad Down
 *   A / Left arrow  = D-Pad Left
 *   D / Right arrow = D-Pad Right
 *   J = B       K = A
 *   Enter = Start      Space = Select
 *
 * NOTE: This module has not been verified on physical hardware.
 * See README "Project status" for details.
 */

#include "main.h"
#include "uart_pad.h"

extern UART_HandleTypeDef huart1;

/* Single-byte RX target for HAL interrupt mode */
static volatile uint8_t rx_byte;
static volatile uint8_t rx_ready;

/* Current pad state: bit0=A bit1=B bit2=Sel bit3=Start
 *                    bit4=Up bit5=Down bit6=Left bit7=Right */
static volatile uint8_t pad_state;

/*-------------------------------------------------------------------*/
/*  Init                                                             */
/*-------------------------------------------------------------------*/

void UART_Pad_Init(void)
{
    rx_ready = 0;
    pad_state = 0;

    /* Start interrupt-driven reception (USART1 must already be
     * initialised by MX_USART1_UART_Init). */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
}

/*-------------------------------------------------------------------*/
/*  Poll — translate a received byte into pad state                  */
/*-------------------------------------------------------------------*/

void UART_Pad_Poll(void)
{
    if (!rx_ready) return;
    rx_ready = 0;

    uint8_t key = rx_byte;

    switch (key) {
        case 'w': case 'W': pad_state |= 0x10; break;   /* Up         */
        case 'a': case 'A': pad_state |= 0x40; break;   /* Left       */
        case 's': case 'S': pad_state |= 0x20; break;   /* Down       */
        case 'd': case 'D': pad_state |= 0x80; break;   /* Right      */
        case 'j': case 'J': pad_state |= 0x02; break;   /* B          */
        case 'k': case 'K': pad_state |= 0x01; break;   /* A          */
        case '\r': case '\n': pad_state |= 0x08; break; /* Start      */
        case ' ':             pad_state |= 0x04; break; /* Select     */
        case '0':             pad_state  = 0;    break; /* Release all*/
        default: return;
    }
}

/*-------------------------------------------------------------------*/
/*  Get                                                              */
/*-------------------------------------------------------------------*/

uint8_t UART_Pad_Get(void)
{
    return pad_state;
}

/*-------------------------------------------------------------------*/
/*  RX callback                                                      */
/*-------------------------------------------------------------------*/

void UART_Pad_RxCallback(uint8_t byte)
{
    rx_byte  = byte;
    rx_ready = 1;

    /* Re-arm for the next byte */
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
}

/* HAL entry point */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        UART_Pad_RxCallback(rx_byte);
    }
}
