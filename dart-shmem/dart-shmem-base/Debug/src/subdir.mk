################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/dart_globals.c \
../src/dart_group.c \
../src/dart_init.c \
../src/dart_locks.c \
../src/dart_logger.c \
../src/dart_malloc.c \
../src/dart_mempool.c \
../src/dart_teams.c 

OBJS += \
./src/dart_globals.o \
./src/dart_group.o \
./src/dart_init.o \
./src/dart_locks.o \
./src/dart_logger.o \
./src/dart_malloc.o \
./src/dart_mempool.o \
./src/dart_teams.o 

C_DEPS += \
./src/dart_globals.d \
./src/dart_group.d \
./src/dart_init.d \
./src/dart_locks.d \
./src/dart_logger.d \
./src/dart_malloc.d \
./src/dart_mempool.d \
./src/dart_teams.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -D_GNU_SOURCE=1 -DDART_DEBUG -DDART_LOG -O0 -g3 -Wall -c -fmessage-length=0 -std=c99 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


