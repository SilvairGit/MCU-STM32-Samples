# ------------------------------------------------------------------------------------------------------------------
# Makefile (based on gcc)
# -------------------------------------------------------------------------------------------------------------------

######################################################################################################################
# Building variables
######################################################################################################################
# debug build
DEBUG = 1
# optimization for debug
# OPT = -Og

# optimization for size
OPT = -Os

######################################################################################################################
# Target
######################################################################################################################
ifndef FW_VERSION
BUILD_NUMBER = x.x.x
else
BUILD_NUMBER = "$(FW_VERSION)"
endif

TARGET= stm32f103_$(BUILD_NUMBER)_debug

######################################################################################################################
# Paths
######################################################################################################################
# Build path
BUILD_DIR = build

######################################################################################################################
# Source
######################################################################################################################
# C sources
C_SOURCES =  \
src/main.c \
hal/SystemHal.c \
hal/PwmHal.c \
hal/TickHal.c \
hal/GpioHal.c \
hal/AdcHal.c \
hal/UartHal.c \
hal/LoggerHal.c \
hal/I2cHal.c \
hal/EncoderHal.c \
hal/FlashHal.c \
hal/Syscalls.c \
hal/AtomicHal.c \
hal/WatchdogHal.c \
simulators/EnergySensorSimulator.c \
simulators/EmergencyDriverSimulator.c \
common/Assert.c \
common/RingBuffer.c \
common/Atomic.c \
common/Timestamp.c \
common/LcdDrv.c \
common/PCF8523Drv.c \
common/Utils.c \
common/Log.c \
common/ButtonDebouncer.c \
common/UartFrame.c \
common/UartProtocol.c \
common/SimpleScheduler.c \
common/ModelManager.c \
common/Checksum.c \
common/Mesh.c \
common/TAILocalTimeConverter.c \
common/Provisioning.c \
features/LCD.c \
features/MCU_DFU.c \
features/MCU_Health.c \
features/OnOffDeltaButtons.c \
features/DeltaEncoder.c \
features/LevelSlider.c \
features/Switch.c \
features/TimeSource.c \
features/TimeReceiver.c \
features/RTC.c \
features/MeshSensorGenerator.c \
features/SensorReceiver.c \
features/PingPong.c \
features/Watchdog.c \
features/Attention.c \
features/EmgLTest.c \
features/Luminaire.c \
stm32f1xx/stm32f1xx_it.c \
stm32f1xx/system_stm32f1xx.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_gpio.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_adc.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_dma.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_rcc.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_utils.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_exti.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_i2c.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_rtc.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_pwr.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_tim.c \
external/STM32F1xx_HAL_Driver/Src/stm32f1xx_ll_usart.c 

# ASM sources
ASM_SOURCES =  \
stm32f1xx/startup_stm32f103xb.s

#######################################################################################################################
# Binaries
#######################################################################################################################
PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
#######################################################################################################################
# CFLAGS
#######################################################################################################################
# cpu
CPU = -mcpu=cortex-m3

# fpu
# NONE for Cortex-M0/M0+/M3

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS = 

# C defines
C_DEFS =  \
-DUSE_FULL_LL_DRIVER \
-DHSE_STARTUP_TIMEOUT=100 \
-DLSE_STARTUP_TIMEOUT=5000 \
-DLSE_VALUE=32768 \
-DHSI_VALUE=8000000 \
-DLSI_VALUE=40000 \
-DVDD_VALUE=3300 \
-DPREFETCH_ENABLE=1 \
-DSTM32F103xB \
-DHSE_VALUE=16000000 \
-DBUILD_NUMBER=\"$(BUILD_NUMBER)\"

# AS includes
AS_INCLUDES = 

# C includes
C_INCLUDES =  \
-Isrc \
-Istm32f1xx \
-Ihal \
-Isimulators \
-Icommon \
-Ifeatures \
-Iexternal/STM32F1xx_HAL_Driver/Inc \
-Iexternal/CMSIS/Device/ST/STM32F1xx/Include \
-Iexternal/CMSIS/Include

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS += $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -ffunction-sections -fdata-sections -fstack-usage 
CFLAGS += -Wdouble-promotion -Wfloat-conversion -Wstrict-prototypes -Wno-discarded-qualifiers -Wextra -Wswitch-default

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################################################################################################
# LDFLAGS
#######################################################################################################################
# link script
LDSCRIPT = stm32f1xx/STM32F103CBUx_FLASH.ld

# libraries
LIBS = -lc -lm -lnosys 
LIBDIR = 
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,--gc-sections

#######################################################################################################################
# Build the application
#######################################################################################################################
BUILD_TARGETS = server client

all: client_cp server_cp

$(BUILD_TARGETS): % : $(BUILD_DIR)/%/$(TARGET).elf $(BUILD_DIR)/%/$(TARGET).hex $(BUILD_DIR)/%/$(TARGET).bin
	
# list of objects
OBJECTS = $(addprefix %/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix %/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/server/%.o: %.c 	
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/server/$(notdir $(<:.c=.lst)) -DMCU_SERVER=1 $< -o $@

$(BUILD_DIR)/server/%.o: %.s 
	mkdir -p $(dir $@)
	$(AS) -c $(ASFLAGS) -DMCU_SERVER=1 $< -o $@

$(BUILD_DIR)/client/%.o: %.c 
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/client/$(notdir $(<:.c=.lst)) -DMCU_CLIENT=1 $< -o $@

$(BUILD_DIR)/client/%.o: %.s 	
	mkdir -p $(dir $@)
	$(AS) -c $(ASFLAGS) -DMCU_CLIENT=1 $< -o $@

%/$(TARGET).elf: $(OBJECTS)	
	$(CC) $^ $(LDFLAGS) -Wl,-Map=$(basename $@).map,--cref -o $@
	$(SZ) $@

%.hex: %.elf
	$(HEX) $< $@
	
%.bin: %.elf
	$(BIN) $< $@

client_cp: client
	for file in $(BUILD_DIR)/client/stm32f103*; do \
		cp -- "$$file" "$(BUILD_DIR)/mcu_client_stm32f103_$${file#$(BUILD_DIR)/client/stm32f103_}"; \
	done

server_cp: server
	for file in $(BUILD_DIR)/server/stm32f103*; do \
		cp -- "$$file" "$(BUILD_DIR)/mcu_server_stm32f103_$${file#$(BUILD_DIR)/server/stm32f103_}"; \
	done

#######################################################################################################################
# Clean up
#######################################################################################################################
clean:
	-rm -fR $(BUILD_DIR)
	$(MAKE) -C test clean

#######################################################################################################################
# Run unit test
#######################################################################################################################
test:
	$(MAKE) -C test test

#######################################################################################################################
# JLink
#######################################################################################################################
flash_server_jlink: $(BUILD_DIR)/server/$(TARGET).hex
	JLinkExe -device "STM32F103CB" -if SWD -speed 4000 -autoconnect 1 -CommanderScript stm32f1xx/flash_server.jlink 
	
flash_client_jlink: $(BUILD_DIR)/client/$(TARGET).hex
	JLinkExe -device "STM32F103CB" -if SWD -speed 4000 -autoconnect 1 -CommanderScript stm32f1xx/flash_client.jlink 

erase_jlink:
	JLinkExe -device "STM32F103CB" -if SWD -speed 4000 -autoconnect 1 -CommanderScript stm32f1xx/erase.jlink 

reset_jlink:
	JLinkExe -device "STM32F103CB" -if SWD -speed 4000 -autoconnect 1 -CommanderScript stm32f1xx/reset.jlink 
	
#######################################################################################################################
# ST-Link
#######################################################################################################################
flash_server_stlink: $(BUILD_DIR)/server/$(TARGET).hex
	STM32_Programmer_CLI -c port=swd freq=4000 --write $< --rst --go

flash_client_stlink: $(BUILD_DIR)/client/$(TARGET).hex
	STM32_Programmer_CLI -c port=swd freq=4000 --write $< --rst --go
	
erase_stlink:
	STM32_Programmer_CLI -c port=swd freq=4000 --erase all 

reset_stlink:
	STM32_Programmer_CLI -c port=swd freq=4000 --rst --go
	
.PHONY: clean test
