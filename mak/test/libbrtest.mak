TARGET = build/libbrtest
SRCS = src/test/test_util.cc
LINK_TYPE = static
INCLUDE = -Isrc
BUILD_DIR = build

include mak/main.mak
