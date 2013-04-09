################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/sysv_barriers.c \
../src/sysv_locks.c \
../src/sysv_memory_manager.c \
../src/sysv_multicast.c 

OBJS += \
./src/sysv_barriers.o \
./src/sysv_locks.o \
./src/sysv_memory_manager.o \
./src/sysv_multicast.o 

C_DEPS += \
./src/sysv_barriers.d \
./src/sysv_locks.d \
./src/sysv_memory_manager.d \
./src/sysv_multicast.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_POSIX_THREAD_PROCESS_SHARED -D_SVID_SOURCE -DDART_LOG -DDART_DEBUG -O0 -g3 -Wall -c -fmessage-length=0 -std=c99 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


