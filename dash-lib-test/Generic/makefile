#############################################################
# simple generic makefile
#############################################################
RM := rm -rf
CC = g++
LD = g++
CFLAGS = -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -Wall -std=c++11
OPTFLAGS = -DDART_DEBUG -DDART_LOG
LIBS := -ldash -ldart-shmem-base -ldart-shmem-sysv -lgtest -lpthread
PROJECT_NAME = test-dash

OBJS += \
./src/NSMPtrTest.o \
./src/Util.o \
./src/dash-lib-test.o \
./src/ArrayTest.o \
./src/group_test.o \
./src/MultiArrayTest.o

-include $(DASH_HOME)/project.mk

