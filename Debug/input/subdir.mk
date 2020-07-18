################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../input/controls.c 

OBJS += \
./input/controls.o 

C_DEPS += \
./input/controls.d 


# Each subdirectory must supply rules for building sources it contributes
input/%.o: ../input/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"/home/nanker/workspace/KetronBPI/include" -I/home/nanker/Embedded/buildroot/output/build/directfb-examples-1.7.0/src -I/home/nanker/Embedded/buildroot/output/build/directfb-1.7.7/lib -I/home/nanker/Embedded/buildroot/output/build/directfb-1.7.7/include -O0 -g3 -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


