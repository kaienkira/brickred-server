include config.mak

TARGET = bin/echo_server
SRCS = src/test/echo_server.cc
LINK_TYPE = exec
INCLUDE = -Isrc
CPP_FLAG = $(BRICKRED_COMPILE_FLAG)
LIB = $(BRICKRED_LINK_FLAG) -Lbuild -lbrickredserver -lbrickredcore -pthread -lrt
DEPFILE = build/libbrickredserver.a
BUILD_DIR = build

include mak/main.mak
