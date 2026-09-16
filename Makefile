# NES emulator for STM32F407VET6
#
# Target: 168 MHz, 512 KB flash, 128 KB SRAM1 + 64 KB CCM
#
# Both paths below can be overridden on the command line, e.g.
#   make TOOLCHAIN=/usr CC=arm-none-eabi-gcc FW=/opt/STM32Cube_FW_F4

# --- Toolchain -----------------------------------------------------
# Defaults to the toolchain bundled with STM32CubeIDE 2.1.1.
# Override with:  make TOOLCHAIN=/usr    (for a system-wide arm-none-eabi-*)
# Note: `:=` rather than `?=` — make predefines CC as "cc", so ?= would
# silently keep the host compiler.
TOOLCHAIN ?= /opt/st/stm32cubeide_2.1.1/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.linux64_1.0.100.202602081740/tools
CC      := $(TOOLCHAIN)/bin/arm-none-eabi-gcc
OBJCOPY := $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
SIZE    := $(TOOLCHAIN)/bin/arm-none-eabi-size

# --- STM32CubeF4 firmware package ----------------------------------
FW ?= $(HOME)/STM32Cube/Repository/STM32Cube_FW_F4_V1.28.3

# --- Compiler flags ------------------------------------------------
CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS  = $(CPU) -DSTM32F407xx -DUSE_HAL_DRIVER
CFLAGS += -O3 -Wall
CFLAGS += -Wno-implicit-function-declaration -Wno-sequence-point
CFLAGS += -ffunction-sections -fdata-sections -flto
CFLAGS += -I. -ICore/Inc -ICore/InfoNES -ICore/InfoNES/mapper -IDrivers/FatFs
CFLAGS += -I$(FW)/Drivers/STM32F4xx_HAL_Driver/Inc
CFLAGS += -I$(FW)/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
CFLAGS += -I$(FW)/Drivers/CMSIS/Device/ST/STM32F4xx/Include
CFLAGS += -I$(FW)/Drivers/CMSIS/Include

LDFLAGS  = $(CPU) -T STM32F407VETX_FLASH.ld -flto
LDFLAGS += -Wl,--gc-sections -Wl,-Map=nes-stm32.map
LDFLAGS += --specs=nosys.specs -lm

# --- Source files --------------------------------------------------

# HAL drivers actually used by this project
HAL_SRC = \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_spi.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_sd.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c \
    $(FW)/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_sdmmc.c

# CMSIS
CMSIS_SRC = Core/system_stm32f4xx.c

# Startup
STARTUP_SRC = $(FW)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f407xx.s

# Application
APP_SRC = \
    Core/Src/main.c \
    Core/Src/nes_port.c \
    Core/Src/rom_data.c \
    Core/Src/uart_pad.c \
    Core/Src/ili9341.c \
    Core/Src/sd_card.c

# InfoNES core (see NOTICE for provenance)
INFONES_SRC = \
    Core/InfoNES/InfoNES.c \
    Core/InfoNES/K6502.c \
    Core/InfoNES/InfoNES_Mapper.c \
    Core/InfoNES/InfoNES_pAPU.c

# FatFs
FATFS_SRC = Drivers/FatFs/ff.c

SRC = $(STARTUP_SRC) $(CMSIS_SRC) $(HAL_SRC) $(APP_SRC) $(INFONES_SRC) $(FATFS_SRC)
OBJ = $(SRC:.c=.o)
OBJ := $(OBJ:.s=.o)

TARGET = nes-stm32
ELF = $(TARGET).elf
BIN = $(TARGET).bin
HEX = $(TARGET).hex

# --- Build rules ---------------------------------------------------

all: $(ELF)

$(ELF): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ)
	$(SIZE) $(ELF)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.s
	$(CC) $(CFLAGS) -c -o $@ $<

# rom_data.c is generated from a ROM you supply, and is not committed.
# This rule turns a missing file into a useful message instead of a
# confusing "no rule to make target".
Core/Src/rom_data.c:
	@echo ""
	@echo "  Core/Src/rom_data.c is missing."
	@echo ""
	@echo "  The ROM is embedded in flash at build time. Generate it from"
	@echo "  a .nes file you have the rights to use:"
	@echo ""
	@echo "      python3 tools/nes2c.py /path/to/game.nes"
	@echo ""
	@exit 1

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

# --- Flashing ------------------------------------------------------
# Adjust the interface config to match your probe:
#   ST-Link : interface/stlink.cfg
#   CMSIS-DAP: interface/cmsis-dap.cfg
PROBE ?= interface/cmsis-dap.cfg

flash: $(ELF)
	openocd -f $(PROBE) -f target/stm32f4x.cfg \
		-c "program $(ELF) verify reset exit"

# --- Clean ---------------------------------------------------------
clean:
	rm -f $(OBJ) $(ELF) $(BIN) $(HEX) nes-stm32.map

.PHONY: all flash clean
