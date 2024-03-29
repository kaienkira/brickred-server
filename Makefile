include config.mak

MAKE = make --no-print-directory

define ECHO
	@printf "\033[;31m"; printf $1; printf "\033[0m\n"
endef

.PHONY: debug release profile clean install

debug release profile clean:
	@$(call ECHO, "[build libbrickred]")
	@$(MAKE) -f mak/libbrickred.mak $@
ifeq ($(BRICKRED_BUILD_TEST), yes)
	@$(call ECHO, "[build libbrtest]")
	@$(MAKE) -f mak/test/libbrtest.mak $@
	@$(call ECHO, "[build testlog]")
	@$(MAKE) -f mak/test/testlog.mak $@
	@$(call ECHO, "[build testrandom]")
	@$(MAKE) -f mak/test/testrandom.mak $@
	@$(call ECHO, "[build testsocket]")
	@$(MAKE) -f mak/test/testsocket.mak $@
	@$(call ECHO, "[build testsocket2]")
	@$(MAKE) -f mak/test/testsocket2.mak $@
	@$(call ECHO, "[build testtimer]")
	@$(MAKE) -f mak/test/testtimer.mak $@
	@$(call ECHO, "[build async_connect]")
	@$(MAKE) -f mak/test/async_connect.mak $@
	@$(call ECHO, "[build async_connect2]")
	@$(MAKE) -f mak/test/async_connect2.mak $@
	@$(call ECHO, "[build base64_encode]")
	@$(MAKE) -f mak/test/base64_encode.mak $@
	@$(call ECHO, "[build base64_decode]")
	@$(MAKE) -f mak/test/base64_decode.mak $@
	@$(call ECHO, "[build broadcast_server]")
	@$(MAKE) -f mak/test/broadcast_server.mak $@
	@$(call ECHO, "[build dns_query]")
	@$(MAKE) -f mak/test/dns_query.mak $@
	@$(call ECHO, "[build echo_client]")
	@$(MAKE) -f mak/test/echo_client.mak $@
	@$(call ECHO, "[build echo_server]")
	@$(MAKE) -f mak/test/echo_server.mak $@
	@$(call ECHO, "[build echo_server2]")
	@$(MAKE) -f mak/test/echo_server2.mak $@
	@$(call ECHO, "[build flash_policy]")
	@$(MAKE) -f mak/test/flash_policy.mak $@
	@$(call ECHO, "[build http_client]")
	@$(MAKE) -f mak/test/http_client.mak $@
	@$(call ECHO, "[build http_server]")
	@$(MAKE) -f mak/test/http_server.mak $@
	@$(call ECHO, "[build md5_sum]")
	@$(MAKE) -f mak/test/md5_sum.mak $@
	@$(call ECHO, "[build md5_sum_binary]")
	@$(MAKE) -f mak/test/md5_sum_binary.mak $@
	@$(call ECHO, "[build port_scan]")
	@$(MAKE) -f mak/test/port_scan.mak $@
	@$(call ECHO, "[build udp_echo_client]")
	@$(MAKE) -f mak/test/sha1_sum.mak $@
	@$(call ECHO, "[build sha1_sum_binary]")
	@$(MAKE) -f mak/test/sha1_sum_binary.mak $@
	@$(call ECHO, "[build sha256_sum]")
	@$(MAKE) -f mak/test/sha256_sum.mak $@
	@$(call ECHO, "[build sha256_sum_binary]")
	@$(MAKE) -f mak/test/sha256_sum_binary.mak $@
	@$(call ECHO, "[build url_encode]")
	@$(MAKE) -f mak/test/udp_echo_client.mak $@
	@$(call ECHO, "[build udp_echo_server]")
	@$(MAKE) -f mak/test/udp_echo_server.mak $@
	@$(call ECHO, "[build url_encode]")
	@$(MAKE) -f mak/test/url_encode.mak $@
	@$(call ECHO, "[build url_decode]")
	@$(MAKE) -f mak/test/url_decode.mak $@
	@$(call ECHO, "[build ws_echo_client]")
	@$(MAKE) -f mak/test/ws_echo_client.mak $@
	@$(call ECHO, "[build ws_echo_server]")
	@$(MAKE) -f mak/test/ws_echo_server.mak $@
endif

install:
	@$(call ECHO, "[install libbrickred]")
	@$(MAKE) -f mak/libbrickred.mak $@
