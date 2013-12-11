#############################################################
# simple generic makefile
#############################################################
RM := rm -rf
CC = gcc
LD = gcc
CFLAGS = -O0 -g3 -Wall -std=c99
OPTFLAGS = -DDART_DEBUG -DDART_LOG
LIBS :=
LIB_NAME = dart-shmem-base

OBJS += \
./src/dart_communication.o \
./src/dart_globals.o \
./src/dart_gptr.o \
./src/dart_group.o \
./src/dart_init.o \
./src/dart_locks.o \
./src/dart_logger.o \
./src/dart_malloc.o \
./src/dart_mempool.o \
./src/dart_teams.o 

-include $(DASH_HOME)/shared_lib.mk

