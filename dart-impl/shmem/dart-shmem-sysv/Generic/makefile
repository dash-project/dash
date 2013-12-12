#############################################################
# simple generic makefile
#############################################################
RM := rm -rf
CC = gcc
LD = gcc
CFLAGS = -O0 -g3 -Wall -std=c99
OPTFLAGS = -D_POSIX_THREAD_PROCESS_SHARED -D_SVID_SOURCE -DDART_DEBUG -DDART_LOG
LIBS := -Wl,-whole-archive "../../dart-shmem-base/Generic/libdart-shmem-base.a" -Wl,-no-whole-archive 
LIB_NAME = dash-rt

OBJS += \
./src/sysv_barriers.o \
./src/sysv_locks.o \
./src/sysv_memory_manager.o \
./src/sysv_multicast.o 

-include $(DASH_HOME)/shared_lib.mk

