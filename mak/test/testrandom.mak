TARGET = bin/testrandom
SRCS = src/test/test_random.cc
LINK_TYPE = exec
INCLUDE = -Isrc
LIB = -Lbuild -lbrickredserver -lbrickredcodec -lbrickredcore -lbrtest -pthread -lrt
DEPFILE = build/libbrickredserver.a build/libbrtest.a
BUILD_DIR = build

include mak/main.mak
