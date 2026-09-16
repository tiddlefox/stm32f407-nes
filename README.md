# NES emulator on an STM32F407

A Nintendo Entertainment System emulator running bare-metal on an
STM32F407VET6, with video out to an ILI9341 TFT over SPI and a ROM
embedded in flash.

No RTOS, no external RAM, no FPGA — just the 192 KB of on-chip RAM and
a display.

<!-- Drop a photo at docs/hardware.jpg and uncomment:
![Hardware running a game](docs/hardware.jpg)
-->

## What works

- Full CPU/PPU/APU emulation via [InfoNES](#third-party-code)
- 256x218 video to an ILI9341, centred on a 320x240 landscape screen
- SPI output driven by DMA, with the CPU free to emulate the next frame
  while the previous one is still being clocked out
- ROM read straight out of flash — it costs no RAM

## Hardware

| Part | Notes |
|------|-------|
| STM32F407VET6 board | 168 MHz, 512 KB flash, 128 KB SRAM + 64 KB CCM |
| ILI9341 TFT, 320x240 | SPI, resistive-touch variants work fine — the touch controller is unused |
| microSD slot | On-board SDIO slot; not required to run (see [Project status](#project-status)) |
| USB-TTL adapter | For gamepad input over UART |

## Wiring

### Display — SPI1

| STM32 | ILI9341 | Notes |
|-------|---------|-------|
| PA5 | SCK | |
| PA7 | SDI / MOSI | |
| PA6 | SDO / MISO | Optional — only needed to read back from the panel |
| PB6 | CS | Software-controlled |
| PB7 | RESET | |
| PB8 | DC / RS | |
| PB9 | LED | Backlight, driven high |
| 3V3 | VCC | |
| GND | GND | |

### Gamepad input — USART1

| STM32 | USB-TTL | Notes |
|-------|---------|-------|
| PA10 | TX | STM32 RX |
| PA9 | RX | Optional, for debug output |
| GND | GND | Required |

### microSD — SDIO

No wiring needed on boards with a built-in slot; it is already connected
to PC8–PC12 and PD2. The driver is present but not used by default — see
[Project status](#project-status).

## Building

Two things are needed beyond this repository: an `arm-none-eabi`
toolchain, and the STM32CubeF4 firmware package for its HAL and CMSIS
sources. The Makefile assumes a system-wide toolchain
(`apt install gcc-arm-none-eabi`) and the standard Cube package location.

If you use STM32CubeIDE, point `TOOLCHAIN` at its bundled compiler —
the directory that contains `bin/arm-none-eabi-gcc`:

```sh
make TOOLCHAIN=/opt/st/<cubeide>/plugins/<gnu-tools-for-stm32>/tools
```

And if your firmware package lives elsewhere:

```sh
make FW=/path/to/STM32Cube_FW_F4_V1.28.3
```

Then:

```sh
python3 tools/nes2c.py /path/to/game.nes    # embeds the ROM (see below)
make
make flash PROBE=interface/stlink.cfg       # or interface/cmsis-dap.cfg
```

## Adding a ROM

**No ROM is bundled with this repository.** Supply your own — and make
sure you have the right to use it; most commercial NES games are still
under copyright.

```sh
python3 tools/nes2c.py /path/to/game.nes
```

That writes `Core/Src/rom_data.c`, which the build embeds into flash. The
file is gitignored. Swap the ROM by re-running the script and reflashing.

The ROM lives in flash, not RAM, and this is load-bearing: the emulator
plus its frame buffer use essentially all 192 KB of RAM, so there is
nowhere to put a ROM in RAM. The 512 KB of flash has plenty of room —
about 390 KB spare after the firmware.

## Controls

Keyboard input is sent one character at a time over UART.

| Key | Button |
|-----|--------|
| `W` `A` `S` `D` | D-Pad |
| `J` | B |
| `K` | A |
| `Enter` | Start |
| `Space` | Select |
| `0` | Release all |

A matching sender is in `tools/pad_serial.py`:

```sh
python3 tools/pad_serial.py /dev/ttyUSB0
```

## Project status

This is a working prototype, not a finished product. Being explicit about
what has actually been verified:

| Part | Status |
|------|--------|
| ILI9341 video over SPI1 + DMA | **Verified on hardware** |
| InfoNES core (CPU, PPU, mappers) | **Verified on hardware** — games run |
| ROM embedded in flash | **Verified on hardware** |
| UART gamepad input | **Not verified.** The STM32-side receive path is standard HAL code, but the host-side serial connection was never brought up on the original build, so treat it as untested |
| SD card driver | SDIO card init succeeds; read/write was never exercised. Not used to load ROMs |
| Audio | **Not implemented.** The APU is stubbed out |

If you get the UART path working (or find it broken), a PR would be
welcome.

## Known limitations

- **The ROM is baked in at build time.** Changing games means
  re-running `nes2c.py` and reflashing. Loading ROMs from the SD card
  would be far nicer, but it does not fit: the frame buffer alone
  occupies 109 KB of the 128 KB SRAM1, leaving no room to stage a ROM.
  Fixing this properly means switching to line-by-line rendering —
  pushing each scanline to the panel as it is drawn instead of buffering
  a whole frame. That frees ~109 KB and also lets the CPU and SPI run in
  parallel. It is the single most valuable change left to make.
- **Mappers are trimmed to 0 (NROM).** `Core/InfoNES/InfoNES_Mapper.c`
  includes only mapper 0; the other 137 mapper files are present but
  `#if 0`-ed out. Several of them declare large static buffers, which
  overflow RAM on this part. Re-enabling them needs those buffers made
  dynamic.
- **Video is cropped.** The panel shows 256x218 rather than the NES's
  256x240. `NES_DISP_HEIGHT` in `Core/InfoNES/InfoNES.h` explains why:
  the frame buffer has to fit in SRAM1.
- **No audio**, and no on-screen menu.

## How it fits in memory

The memory split is the most interesting constraint in this project, and
it drives most of the design decisions above.

```
Flash  512 KB
  ├── firmware          ~119 KB
  └── ROM               ~390 KB free

SRAM1  128 KB                     CCM  64 KB (CPU only, no DMA)
  ├── WorkFrame  109 KB             ├── ChrBuf    32 KB
  ├── K6502 tables 3 KB             ├── PPURAM    16 KB
  └── stacks, buffers               ├── RAM        8 KB
                                    └── SRAM       8 KB
```

Two consequences worth knowing:

- **`WorkFrame` must live in SRAM1**, because the SPI DMA engine cannot
  reach CCM. That is why the frame buffer dictates the video height.
- **The ROM must live in flash**, because there is no RAM left for it.

## Third-party code

| Component | Origin | Licence |
|-----------|--------|---------|
| InfoNES | TMK's InfoNES, via the [PSP port](https://github.com/PSP-Archive/InfoNES) | Freeware; no formal licence |
| FatFs | ChaN | BSD-style |
| STM32 HAL / CMSIS | STMicroelectronics | BSD-3-Clause |

See [NOTICE](NOTICE) for detail, including the changes made to InfoNES to
get it running on this target.

## Licence

MIT — see [LICENSE](LICENSE). The third-party components listed above
remain under their own terms.
