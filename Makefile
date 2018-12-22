#CC = gcc
SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "obj") print $$9}')

ROOT_DIR=$(shell pwd)
BIN=main
OBJS_DIR=obj/tmp
BIN_DIR=obj/bin

CUR_SOURCE := ${wildcard *.c}
#CUR_SOURCE += ${wildcard ./net/*.c ./videoproto/*.c ./queue/*.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
CUR_INCFLAGS := -Iinclude
export CC BIN OBJS_DIR BIN_DIR ROOT_DIR

ifneq (debug,0)
	BUILD_FLAG += -g
endif



all: $(BIN)

.PHONY : all clean

$(BIN):$(CUR_OBJS)
	$(CC) $^ -o $@ -lpthread 
# -lrt

$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $@ $(CUR_INCFLAGS) $(BUILD_FLAG) -Wall -O2 
	
clean:
	@rm -rf $(CUR_OBJS) $(BIN)
