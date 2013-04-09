################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/InitTest.cpp \
../src/LocksTest.cpp \
../src/MempoolTest.cpp \
../src/Util.cpp \
../src/main.cpp 

OBJS += \
./src/InitTest.o \
./src/LocksTest.o \
./src/MempoolTest.o \
./src/Util.o \
./src/main.o 

CPP_DEPS += \
./src/InitTest.d \
./src/LocksTest.d \
./src/MempoolTest.d \
./src/Util.d \
./src/main.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -D__GXX_EXPERIMENTAL_CXX0X__ -DDART_LOG -DDART_DEBUG -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


