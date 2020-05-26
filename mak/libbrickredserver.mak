include config.mak

TARGET = build/libbrickredserver
SRCS = \
src/brickred/condition_variable.cc \
src/brickred/io_device.cc \
src/brickred/io_service.cc \
src/brickred/log_core.cc \
src/brickred/log_async_sink.cc \
src/brickred/log_file_sink.cc \
src/brickred/log_stderr_sink.cc \
src/brickred/mutex.cc \
src/brickred/random.cc \
src/brickred/self_pipe.cc \
src/brickred/signal_queue.cc \
src/brickred/socket_address.cc \
src/brickred/system.cc \
src/brickred/tcp_socket.cc \
src/brickred/tcp_service.cc \
src/brickred/thread.cc \
src/brickred/timer_heap.cc \
src/brickred/timestamp.cc \
src/brickred/udp_socket.cc \
src/brickred/protocol/http_message.cc \
src/brickred/protocol/http_protocol.cc \
src/brickred/protocol/http_request.cc \
src/brickred/protocol/http_response.cc \
src/brickred/protocol/web_socket_protocol.cc \

LINK_TYPE = static
INCLUDE = -Isrc
CPP_FLAG = $(BRICKRED_COMPILE_FLAG)
BUILD_DIR = build

include mak/main.mak

.PHONY: install

install:
	@mkdir -p "$(BRICKRED_INSTALL_PREFIX)"/include/brickred
	@mkdir -p "$(BRICKRED_INSTALL_PREFIX)"/include/brickred/protocol
	@cp src/brickred/*.h \
		"$(BRICKRED_INSTALL_PREFIX)"/include/brickred
	@cp src/brickred/protocol/*.h \
		"$(BRICKRED_INSTALL_PREFIX)"/include/brickred/protocol
	@mkdir -p "$(BRICKRED_INSTALL_PREFIX)"/lib
	@cp "$(FINAL_TARGET)" "$(BRICKRED_INSTALL_PREFIX)"/lib
