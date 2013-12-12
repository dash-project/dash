#############################################################
# simple generic makefile
#############################################################
RM := rm -rf
CC = g++
LD = g++
CFLAGS = -D__GXX_EXPERIMENTAL_CXX0X__ -O0 -g3 -Wall -std=c++11
OPTFLAGS = -DDART_DEBUG -DDART_LOG
LIBS :=
LIB_NAME = dash

OBJS += \
./src/dummy.o \
./src/group.o \
./src/team.o \
./src/unit.o

-include $(DASH_HOME)/shared_lib.mk

