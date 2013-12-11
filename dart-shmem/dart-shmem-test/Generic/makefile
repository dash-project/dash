#############################################################
# simple generic makefile
#############################################################
RM := rm -rf
CC = g++
LD = g++
CFLAGS = -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -Wall -std=c++11
OPTFLAGS = -DDART_DEBUG -DDART_LOG
LIBS := -ldart-shmem-base -ldart-shmem-sysv -lgtest -lpthread
PROJECT_NAME = test-shmem

OBJS += \
./src/InitTest.o \
./src/LocksTest.o \
./src/MempoolTest.o \
./src/TeamTest.o \
./src/Util.o \
./src/main.o 

-include $(DASH_HOME)/project.mk

