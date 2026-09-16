################################################################################
# Makefile for STM32H753ZI MicroSD (SDMMC1 4-bit) & VCP Project
################################################################################

TARGET = h753zi_sd
BUILD_DIR = build

# Toolchain paths
GCC_PATH = C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin

CC = "$(GCC_PATH)/arm-none-eabi-gcc"
AS = "$(GCC_PATH)/arm-none-eabi-gcc" -x assembler-with-cpp
CP = "$(GCC_PATH)/arm-none-eabi-objcopy"
SZ = "$(GCC_PATH)/arm-none-eabi-size"
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# CPU and FPU flags
CPU = -mcpu=cortex-m7
FPU = -mfpu=fpv5-d16
FLOAT-ABI = -mfloat-abi=hard
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# C Sources
C_SOURCES =  \
Src/main.c \
Src/stm32h7xx_it.c \
Src/stm32h7xx_hal_msp.c \
Src/system_stm32h7xx.c \
Src/syscalls.c \
Src/sysmem.c \
Src/sd_diskio.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_mdma.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_flash_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_sd.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_ll_sdmmc.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth.c \
Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_eth_ex.c \
Middlewares/FatFs/src/ff.c \
Drivers/BSP/Components/lan8742/lan8742.c \
Src/ethernetif.c \
Src/app_ethernet.c \
Middlewares/LwIP/src/core/init.c \
Middlewares/LwIP/src/core/def.c \
Middlewares/LwIP/src/core/dns.c \
Middlewares/LwIP/src/core/inet_chksum.c \
Middlewares/LwIP/src/core/ip.c \
Middlewares/LwIP/src/core/mem.c \
Middlewares/LwIP/src/core/memp.c \
Middlewares/LwIP/src/core/netif.c \
Middlewares/LwIP/src/core/pbuf.c \
Middlewares/LwIP/src/core/raw.c \
Middlewares/LwIP/src/core/stats.c \
Middlewares/LwIP/src/core/sys.c \
Middlewares/LwIP/src/core/tcp.c \
Middlewares/LwIP/src/core/tcp_in.c \
Middlewares/LwIP/src/core/tcp_out.c \
Middlewares/LwIP/src/core/timeouts.c \
Middlewares/LwIP/src/core/udp.c \
Middlewares/LwIP/src/core/ipv4/autoip.c \
Middlewares/LwIP/src/core/ipv4/dhcp.c \
Middlewares/LwIP/src/core/ipv4/etharp.c \
Middlewares/LwIP/src/core/ipv4/icmp.c \
Middlewares/LwIP/src/core/ipv4/igmp.c \
Middlewares/LwIP/src/core/ipv4/ip4.c \
Middlewares/LwIP/src/core/ipv4/ip4_addr.c \
Middlewares/LwIP/src/core/ipv4/ip4_frag.c \
Middlewares/LwIP/src/netif/ethernet.c

# ASM Sources
ASM_SOURCES =  \
Startup/startup_stm32h753zitx.s

# C Defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32H753xx

# Include Paths
C_INCLUDES =  \
-IInc \
-IDrivers/STM32H7xx_HAL_Driver/Inc \
-IDrivers/STM32H7xx_HAL_Driver/Inc/Legacy \
-IDrivers/CMSIS/Device/ST/STM32H7xx/Include \
-IDrivers/CMSIS/Include \
-IMiddlewares/FatFs/src \
-IMiddlewares/LwIP/src/include \
-IDrivers/BSP/Components/lan8742

# Compile Flags
CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) -Og -Wall -fdata-sections -ffunction-sections -g -gdwarf-2

# Linker script
LDSCRIPT = STM32H753ZITX_FLASH.ld

# Linker flags
LIBS = -lc -lm -lnosys
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

# Objects list
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@
	
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@	
	
$(BUILD_DIR):
	cmd.exe /c "if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)"

clean:
	cmd.exe /c "if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)"

flash: all
	cmd.exe /c copy /Y $(BUILD_DIR)\\$(TARGET).bin D:\\

.PHONY: all clean flash
