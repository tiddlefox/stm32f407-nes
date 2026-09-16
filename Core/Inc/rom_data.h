/**
 * rom_data.h - Embedded NES ROM
 *
 * Core/Src/rom_data.c is generated at build time by tools/nes2c.py from
 * a .nes file you supply. It is deliberately NOT committed to this
 * repository — see "Adding a ROM" in the README.
 *
 * Keeping the ROM in flash (rather than RAM) is what makes this fit:
 * the emulator aliases straight into the memory-mapped flash region,
 * so the ROM costs no RAM at all.
 */

#ifndef ROM_DATA_H
#define ROM_DATA_H

extern const unsigned char nes_rom_data[];
extern const unsigned int  nes_rom_size;

#endif /* ROM_DATA_H */
