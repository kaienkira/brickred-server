include config.mak

TARGET = bin/url_decode
SRCS = src/test/url_decode.cc
LINK_TYPE = exec
INCLUDE = $(BRICKRED_COMPILE_FLAG) -Isrc
LIB = $(BRICKRED_LINK_FLAG) -Lbuild -lbrickred -pthread -lrt
DEPFILE = build/libbrickred.a
BUILD_DIR = build

include mak/main.mak
