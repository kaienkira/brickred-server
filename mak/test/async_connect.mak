include config.mak

TARGET = bin/async_connect
SRCS = src/test/async_connect.cc
LINK_TYPE = exec
INCLUDE = -Isrc
CPP_FLAG = $(BRICKRED_COMPILE_FLAG)
LIB = $(BRICKRED_LINK_FLAG) -Lbuild -lbrickred -pthread -lrt
DEPFILE = build/libbrickred.a
BUILD_DIR = build

include mak/main.mak
