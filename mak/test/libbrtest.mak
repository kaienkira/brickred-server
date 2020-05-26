include config.mak

TARGET = build/libbrtest
SRCS = src/test/test_util.cc
LINK_TYPE = static
INCLUDE = -Isrc
CPP_FLAG = $(BRICKRED_COMPILE_FLAG)
BUILD_DIR = build

include mak/main.mak
