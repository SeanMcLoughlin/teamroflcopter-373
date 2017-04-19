################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Adafruit_GFX.cpp \
../src/Adafruit_ILI9341.cpp \
../src/cr_cpp_config.cpp \
../src/cr_startup_lpc175x_6x.cpp \
../src/lcd.cpp 

C_SRCS += \
../src/crp.c \
../src/glcdfont.c \
../src/sysinit.c 

OBJS += \
./src/Adafruit_GFX.o \
./src/Adafruit_ILI9341.o \
./src/cr_cpp_config.o \
./src/cr_startup_lpc175x_6x.o \
./src/crp.o \
./src/glcdfont.o \
./src/lcd.o \
./src/sysinit.o 

CPP_DEPS += \
./src/Adafruit_GFX.d \
./src/Adafruit_ILI9341.d \
./src/cr_cpp_config.d \
./src/cr_startup_lpc175x_6x.d \
./src/lcd.d 

C_DEPS += \
./src/crp.d \
./src/glcdfont.d \
./src/sysinit.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -D__NEWLIB__ -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC17XX__ -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_board_nxp_lpcxpresso_1769/inc" -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_chip_175x_6x/inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__NEWLIB__ -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC17XX__ -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_board_nxp_lpcxpresso_1769/inc" -I"/Users/zihanli/program/LPCXpresso_8.2.2/workspace/lpc_chip_175x_6x/inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


