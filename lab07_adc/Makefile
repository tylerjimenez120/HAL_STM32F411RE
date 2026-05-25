PREFIX  ?= /opt/gcc-arm/bin/arm-none-eabi
CC       = $(PREFIX)-gcc
AS       = $(PREFIX)-gcc -x assembler-with-cpp
OBJCOPY  = $(PREFIX)-objcopy
SIZE     = $(PREFIX)-size

TARGET   = lab07_adc

CUBE    ?= $(HOME)/stm32/hal_stm32/STM32CubeF4
HAL_SRC  = $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Src
HAL_INC  = $(CUBE)/Drivers/STM32F4xx_HAL_Driver/Inc
CMSIS_INC  = $(CUBE)/Drivers/CMSIS/Include
DEVICE_INC = $(CUBE)/Drivers/CMSIS/Device/ST/STM32F4xx/Include

C_SRCS = \
    Src/main.c \
    $(CUBE)/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
    $(HAL_SRC)/stm32f4xx_hal.c \
    $(HAL_SRC)/stm32f4xx_hal_rcc.c \
    $(HAL_SRC)/stm32f4xx_hal_rcc_ex.c \
    $(HAL_SRC)/stm32f4xx_hal_gpio.c \
    $(HAL_SRC)/stm32f4xx_hal_cortex.c \
    $(HAL_SRC)/stm32f4xx_hal_pwr.c \
    $(HAL_SRC)/stm32f4xx_hal_pwr_ex.c \
    $(HAL_SRC)/stm32f4xx_hal_flash.c \
    $(HAL_SRC)/stm32f4xx_hal_flash_ex.c \
    $(HAL_SRC)/stm32f4xx_hal_uart.c \
    $(HAL_SRC)/stm32f4xx_hal_dma.c \
    $(HAL_SRC)/stm32f4xx_hal_dma_ex.c \
    $(HAL_SRC)/stm32f4xx_hal_adc.c \
    $(HAL_SRC)/stm32f4xx_hal_adc_ex.c

AS_SRCS = startup_stm32f411xe.s

ARCH = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

CFLAGS  = $(ARCH) -O0 -g -Wall
CFLAGS += -DSTM32F411xE
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += -I Inc
CFLAGS += -I $(HAL_INC)
CFLAGS += -I $(CMSIS_INC)
CFLAGS += -I $(DEVICE_INC)
CFLAGS += -ffunction-sections -fdata-sections

LDFLAGS  = $(ARCH)
LDFLAGS += -T STM32F411RETX_FLASH.ld
LDFLAGS += --specs=nano.specs
LDFLAGS += --specs=nosys.specs
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,-Map=$(TARGET).map

C_OBJS  = $(C_SRCS:.c=.o)
AS_OBJS = $(AS_SRCS:.s=.o)
OBJS    = $(C_OBJS) $(AS_OBJS)

all: $(TARGET).bin

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(AS) $(ARCH) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@
	$(SIZE) $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(TARGET).bin
	openocd -f interface/stlink.cfg \
	        -f target/stm32f4x.cfg \
	        -c "program $(TARGET).bin verify reset exit 0x08000000"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map

.PHONY: all flash clean