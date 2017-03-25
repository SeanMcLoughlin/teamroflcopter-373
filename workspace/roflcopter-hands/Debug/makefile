################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include example/src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: roflcopter-hands.axf

# Tool invocations
roflcopter-hands.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/smclough/LPCXpresso/workspace/lpc_chip_175x_6x/Debug" -L"/home/smclough/LPCXpresso/workspace/lpc_board_nxp_lpcxpresso_1769/Debug" -Xlinker -Map="roflcopter-hands.map" -Xlinker --gc-sections -mcpu=cortex-m3 -mthumb -T "periph_uart_rb_Debug.ld" -o "roflcopter-hands.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) roflcopter-hands.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "roflcopter-hands.axf"; # arm-none-eabi-objcopy -v -O binary "roflcopter-hands.axf" "roflcopter-hands.bin" ; # checksum -p LPC1769 -d "roflcopter-hands.bin";
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
