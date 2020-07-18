################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/MD_MIDIFile.c \
../src/MD_MIDIHelper.c \
../src/MD_MIDITrack.c \
../src/main.c \
../src/midi.c \
../src/sounds.c 

OBJS += \
./src/MD_MIDIFile.o \
./src/MD_MIDIHelper.o \
./src/MD_MIDITrack.o \
./src/main.o \
./src/midi.o \
./src/sounds.o 

C_DEPS += \
./src/MD_MIDIFile.d \
./src/MD_MIDIHelper.d \
./src/MD_MIDITrack.d \
./src/main.d \
./src/midi.d \
./src/sounds.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -I"/home/nanker/workspace/KetronBPI/include" -I/home/nanker/Embedded/buildroot/output/build/directfb-examples-1.7.0/src -I/home/nanker/Embedded/buildroot/output/build/directfb-1.7.7/lib -I/home/nanker/Embedded/buildroot/output/build/directfb-1.7.7/include -O0 -g3 -Wall -Werror -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


