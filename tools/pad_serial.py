#!/usr/bin/env python3
"""
pad_serial.py - Send gamepad input to the STM32 over a serial link.

Opens a small window to capture real key-down/key-up events (terminals
cannot see key releases, so a window is the simplest portable way to
support holding a direction, or running and jumping at once), and sends
one character per event to the board.

The firmware side is Core/Src/uart_pad.c.

Requires pyserial and pygame:
    pip install pyserial pygame

Usage:
    python3 tools/pad_serial.py [port]

If no port is given, the first /dev/ttyUSB* or /dev/ttyACM* is used.
"""

import glob
import sys

import pygame
import serial

BAUD = 115200

# Key -> character the firmware expects
KEY_MAP = {
    pygame.K_UP:    b'w',  pygame.K_w: b'w',
    pygame.K_DOWN:  b's',  pygame.K_s: b's',
    pygame.K_LEFT:  b'a',  pygame.K_a: b'a',
    pygame.K_RIGHT: b'd',  pygame.K_d: b'd',
    pygame.K_j:     b'j',           # B
    pygame.K_k:     b'k',           # A
    pygame.K_RETURN: b'\r',         # Start
    pygame.K_SPACE:  b' ',          # Select
}

LABELS = {
    b'w': 'UP', b's': 'DOWN', b'a': 'LEFT', b'd': 'RIGHT',
    b'j': 'B', b'k': 'A', b'\r': 'START', b' ': 'SELECT',
}


def find_port():
    ports = sorted(glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyUSB*"))
    if not ports:
        print("No serial port found. Pass one explicitly:")
        print("    python3 tools/pad_serial.py /dev/ttyUSB0")
        sys.exit(1)
    return ports[0]


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()

    try:
        ser = serial.Serial(port, BAUD, timeout=0.1)
    except serial.SerialException as e:
        print(f"Cannot open {port}: {e}")
        return 1

    print(f"Opened {port} at {BAUD} baud.")

    pygame.init()
    screen = pygame.display.set_mode((340, 90))
    pygame.display.set_caption("NES gamepad - click here, then press keys")
    font = pygame.font.SysFont("monospace", 16)
    clock = pygame.time.Clock()

    print("Click the window, then use:")
    print("  WASD / arrows = D-Pad    J = B    K = A")
    print("  Enter = Start    Space = Select    Esc = quit")

    held = set()
    running = True

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    running = False
                    break
                key = KEY_MAP.get(event.key)
                if key:
                    held.add(key)
                    ser.write(key)

            elif event.type == pygame.KEYUP:
                key = KEY_MAP.get(event.key)
                if key and key in held:
                    held.discard(key)
                    # The firmware latches presses until told otherwise,
                    # so release by re-sending whatever is still held, or
                    # an explicit clear.
                    if held:
                        ser.write(b'0')
                        for k in held:
                            ser.write(k)
                    else:
                        ser.write(b'0')

        screen.fill((18, 18, 30))
        text = " + ".join(LABELS[k] for k in held) if held else "press keys"
        screen.blit(font.render(f"Keys: {text}", True, (90, 230, 120)), (12, 34))
        pygame.display.flip()
        clock.tick(30)

    pygame.quit()
    ser.write(b'0')
    ser.close()
    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
