#include <stdint.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "test/test_util.h"
#include <brickred/command_line_option.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/string_util.h>
#include <brickred/tcp_service.h>
#include <brickred/protocol/http_protocol.h>
#include <brickred/protocol/http_request.h>
#include <brickred/protocol/http_response.h>

using namespace brickred;
using namespace brickred::protocol;

static bool g_opt_print_hex = false;
static std::string g_opt_host;
static std::string g_opt_user_agent;

class HttpClient {
public:
    HttpClient() : tcp_service_(io_service_), socket_id_(-1)
    {
        tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpClient::onRecvMessage, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpClient::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &HttpClient::onError, this));
    }

    ~HttpClient()
    {
    }

    bool request(const SocketAddress &addr, const std::string &request_uri)
    {
        socket_id_ = tcp_service_.connect(addr);
        if (-1 == socket_id_) {
            ::fprintf(stderr, "[error] socket connect failed: %s\n",
                     ::strerror(errno));
            return false;
        }

        sendHttpRequest(addr, request_uri);
        io_service_.loop();

        return true;
    }

    void sendHttpRequest(const SocketAddress &addr,
                         const std::string &request_uri)
    {
        HttpRequest request;
        request.setVersion(HttpMessage::Version::HTTP_1_1);
        request.setMethod(HttpRequest::Method::GET);
        request.setRequestUri(request_uri);

        if (g_opt_host.empty() == false) {
            request.setHeader("Host", g_opt_host);
        } else {
            request.setHeader("Host",
                addr.getIp() + ":" + string_util::toString(addr.getPort()));
        }
        if (g_opt_user_agent.empty() == false) {
            request.setHeader("User-Agent", g_opt_user_agent);
        }

        request.setHeader("Connection", "keep-alive");

        DynamicBuffer buffer;
        HttpProtocol::writeMessage(request, &buffer);

        tcp_service_.sendMessage(socket_id_,
            buffer.readBegin(), buffer.readableBytes());
    }

    void printHttpResponse(const HttpResponse &response)
    {
        ::printf("[recv http response] %d %s\n",
                 response.getStatusCode(),
                 response.getReasonPhrase().c_str());

        // print header
        ::printf("--- header ---\n");
        for (HttpMessage::HeaderMap::const_iterator iter =
                 response.getHeaders().begin();
             iter != response.getHeaders().end(); ++iter) {
            ::printf("    %s: %s\n",
                     iter->first.c_str(), iter->second.c_str());
        }
        // print body
        ::printf("\n--- body ---\n\n");
        if (g_opt_print_hex) {
            test::hexdump(response.getBody().c_str(),
                          response.getBody().size());
        } else {
            ::printf("%s\n", response.getBody().c_str());
        }
    }

    void onRecvMessage(TcpService *service,
                       TcpService::SocketId socket_id,
                       DynamicBuffer *buffer)
    {
        HttpProtocol::RetCode::type ret = protocol_.recvMessage(buffer);
        if (HttpProtocol::RetCode::WAITING_MORE_DATA == ret) {
            return;

        } else if (HttpProtocol::RetCode::ERROR == ret) {
            ::printf("[error] recieve Message failed\n");
            quit();

        } else if (HttpProtocol::RetCode::MESSAGE_READY == ret) {
            HttpResponse response;
            if (protocol_.retrieveResponse(&response) == false) {
                ::printf("[error] retrieve response failed\n");
                quit();
            }
            printHttpResponse(response);
            quit();
            
        } else {
            quit();
        }
    }

    void onPeerClose(TcpService *service,
                     TcpService::SocketId socket_id)
    {
        ::printf("[peer close] %lx\n", socket_id);
        quit();
    }

    void onError(TcpService *service,
                 TcpService::SocketId socket_id,
                 int error)
    {
        ::printf("[error] %lx: %s\n", socket_id, ::strerror(error));
        quit();
    }

    void quit()
    {
        tcp_service_.closeSocket(socket_id_);
        io_service_.quit();
    }

private:
    IOService io_service_;
    TcpService tcp_service_;
    TcpService::SocketId socket_id_;
    HttpProtocol protocol_;
};

static void printUsage(const char *progname)
{
    ::fprintf(stderr, "usage: %s <ip>\n"
              "[-p <port>] [-r <request_uri>]\n"
              "[-H(hex_output)]\n"
              "[--user-agent <user_agent>]\n"
              "[--host <host>]\n",
              progname);
}

int main(int argc, char *argv[])
{
    std::string ip;
    uint16_t port = 80;
    std::string request_uri = "/";

    CommandLineOption options;
    options.addOption("p", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("r", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("H");
    options.addOption("host", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("user-agent",
                      CommandLineOption::ParameterType::REQUIRED);

    if (options.parse(argc, argv) == false) {
        printUsage(argv[0]);
        return -1;
    }
    if (options.hasOption("p")) {
        port = ::atoi(options.getParameter("p").c_str());
    }
    if (options.hasOption("r")) {
        request_uri = options.getParameter("r");
    }
    if (options.hasOption("H")) {
        g_opt_print_hex = true;
    }
    if (options.hasOption("host")) {
        g_opt_host = options.getParameter("host");
    }
    if (options.hasOption("user-agent")) {
        g_opt_user_agent = options.getParameter("user-agent");
    }
    if (options.getLeftArguments().size() != 1) {
        printUsage(argv[0]);
        return -1;
    }
    ip = options.getLeftArguments()[0];

    HttpClient client;
    client.request(SocketAddress(ip, port), request_uri);

    return 0;
}
