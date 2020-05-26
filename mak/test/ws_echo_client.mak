include config.mak

TARGET = bin/ws_echo_client
SRCS = src/test/ws_echo_client.cc
LINK_TYPE = exec
INCLUDE = -Isrc
CPP_FLAG = $(BRICKRED_COMPILE_FLAG)
LIB = $(BRICKRED_LINK_FLAG) -Lbuild -lbrickredserver \
-lbrickredcodec -lbrickredcore -lbrtest -pthread -lrt
DEPFILE = build/libbrickredserver.a build/libbrtest.a
BUILD_DIR = build

include mak/main.mak
