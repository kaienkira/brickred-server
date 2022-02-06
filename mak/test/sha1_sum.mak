include config.mak

TARGET = bin/sha1_sum
SRCS = src/test/sha1_sum.cc
LINK_TYPE = exec
INCLUDE = $(BRICKRED_COMPILE_FLAG) -Isrc
LIB = $(BRICKRED_LINK_FLAG) -Lbuild -lbrickred -pthread -lrt
DEPFILE = build/libbrickred.a
BUILD_DIR = build

include mak/main.mak
