TARGET = bin/http_server
SRCS = src/test/http_server.cc
LINK_TYPE = exec
INCLUDE = -Isrc
LIB = -Lbuild -lbrickredserver -lbrickredcore -lbrtest -pthread -lrt
DEPFILE = build/libbrickredserver.a build/libbrtest.a
BUILD_DIR = build

include mak/main.mak
