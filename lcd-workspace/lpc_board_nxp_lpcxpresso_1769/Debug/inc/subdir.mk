################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../inc/hal.c \
../inc/lpc17xx_adc.c \
../inc/lpc17xx_clkpwr.c \
../inc/lpc17xx_gpdma.c \
../inc/lpc17xx_gpio.c \
../inc/lpc17xx_pinsel.c \
../inc/lpc17xx_pwm.c \
../inc/lpc17xx_ssp.c \
../inc/lpc17xx_timer.c \
../inc/lpc17xx_uart.c 

OBJS += \
./inc/hal.o \
./inc/lpc17xx_adc.o \
./inc/lpc17xx_clkpwr.o \
./inc/lpc17xx_gpdma.o \
./inc/lpc17xx_gpio.o \
./inc/lpc17xx_pinsel.o \
./inc/lpc17xx_pwm.o \
./inc/lpc17xx_ssp.o \
./inc/lpc17xx_timer.o \
./inc/lpc17xx_uart.o 

C_DEPS += \
./inc/hal.d \
./inc/lpc17xx_adc.d \
./inc/lpc17xx_clkpwr.d \
./inc/lpc17xx_gpdma.d \
./inc/lpc17xx_gpio.d \
./inc/lpc17xx_pinsel.d \
./inc/lpc17xx_pwm.d \
./inc/lpc17xx_ssp.d \
./inc/lpc17xx_timer.d \
./inc/lpc17xx_uart.d 


# Each subdirectory must supply rules for building sources it contributes
inc/%.o: ../inc/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DDEBUG -D__CODE_RED -D__USE_LPCOPEN -DCORE_M3 -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_chip_175x_6x/inc" -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_board_nxp_lpcxpresso_1769/inc" -O0 -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


