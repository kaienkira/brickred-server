#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <brickred/class_util.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/message_queue.h>
#include <brickred/tcp_service.h>
#include <brickred/socket_address.h>
#include <brickred/thread.h>

using namespace brickred;

class Message {
public:
    TcpService::SocketId socket_id_;
    std::string message_;
};

class NetworkThread {
public:
    bool start(const SocketAddress &addr);
    void join();
    void sendMessage(const Message &message);

private:
    void run();
    void onNetNewConnection(TcpService *service,
                            TcpService::SocketId from_socket_id,
                            TcpService::SocketId socket_id);
    void onNetRecvMessage(TcpService *service,
                          TcpService::SocketId socket_id,
                          DynamicBuffer *buffer);
    void onNetPeerClose(TcpService *service,
                        TcpService::SocketId socket_id);
    void onNetError(TcpService *service,
                    TcpService::SocketId socket_id,
                    int error);
    void onSendMessage(MessageQueue<Message> *queue);

private:
    BRICKRED_SINGLETON(NetworkThread)

    Thread thread_;
    IOService io_service_;
    TcpService tcp_service_;
    MessageQueue<Message> message_queue_;
};

class LogicThread {
public:
    void start();
    void join();
    void processMessage(const Message &message);

private:
    void run();
    void onProcessMessage(MessageQueue<Message> *queue);

private:
    BRICKRED_SINGLETON(LogicThread)

    Thread thread_;
    IOService io_service_;
    MessageQueue<Message> message_queue_;
};

///////////////////////////////////////////////////////////////////////////////
NetworkThread::NetworkThread() :
    tcp_service_(io_service_), message_queue_(io_service_)
{
    tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
        &NetworkThread::onNetNewConnection, this));
    tcp_service_.setRecvMessageCallback(BRICKRED_BIND_MEM_FUNC(
        &NetworkThread::onNetRecvMessage, this));
    tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
        &NetworkThread::onNetPeerClose, this));
    tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
        &NetworkThread::onNetError, this));
    message_queue_.setRecvMessageCallback(
        BRICKRED_BIND_MEM_FUNC(&NetworkThread::onSendMessage, this));
}

NetworkThread::~NetworkThread()
{
}

bool NetworkThread::start(const SocketAddress &addr)
{
    if (tcp_service_.listen(addr) < 0) {
        ::fprintf(stderr, "socket listen failed: %s\n",
                  ::strerror(errno));
        return false;
    }

    thread_.start(BRICKRED_BIND_MEM_FUNC(
        &NetworkThread::run, this));

    return true;
}

void NetworkThread::join()
{
    thread_.join();
}

void NetworkThread::sendMessage(const Message &message)
{
    message_queue_.push(message);
}

void NetworkThread::run()
{
    io_service_.loop();
}

void NetworkThread::onNetNewConnection(TcpService *service,
                                       TcpService::SocketId from_socket_id,
                                       TcpService::SocketId socket_id)
{
    static int conn_num = 0;
    ::printf("[new connection][%d] %lx from %lx\n",
             ++conn_num, socket_id, from_socket_id);
}

void NetworkThread::onNetRecvMessage(TcpService *service,
                                     TcpService::SocketId socket_id,
                                     DynamicBuffer *buffer)
{
    std::string buffer_string(buffer->readBegin(), buffer->readableBytes());
    buffer->read(buffer->readableBytes());

    Message message;
    message.socket_id_ = socket_id;
    message.message_ = buffer_string;

    LogicThread::getInstance()->processMessage(message);
}

void NetworkThread::onNetPeerClose(TcpService *service,
                                   TcpService::SocketId socket_id)
{
    ::printf("[peer close] %lx\n", socket_id);
    service->closeSocket(socket_id);
}

void NetworkThread::onNetError(TcpService *service,
                               TcpService::SocketId socket_id,
                               int error)
{
    ::printf("[error] %lx: %s\n", socket_id, ::strerror(error));
    service->closeSocket(socket_id);
}

void NetworkThread::onSendMessage(MessageQueue<Message> *queue)
{
    Message message;
    while (queue->pop(message)) {
        tcp_service_.sendMessage(message.socket_id_,
            message.message_.c_str(), message.message_.size());
    }
}

///////////////////////////////////////////////////////////////////////////////
LogicThread::LogicThread() :
    message_queue_(io_service_)
{
    message_queue_.setRecvMessageCallback(
        BRICKRED_BIND_MEM_FUNC(&LogicThread::onProcessMessage, this));
}

LogicThread::~LogicThread()
{
}

void LogicThread::start()
{
    thread_.start(BRICKRED_BIND_MEM_FUNC(
        &LogicThread::run, this));
}

void LogicThread::join()
{
    thread_.join();
}

void LogicThread::processMessage(const Message &message)
{
    message_queue_.push(message);
}

void LogicThread::run()
{
    io_service_.loop();
}

void LogicThread::onProcessMessage(MessageQueue<Message> *queue)
{
    Message message;
    while (queue->pop(message)) {
        NetworkThread::getInstance()->sendMessage(message);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    if (argc < 3) {
        ::fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        return -1;
    }

    if (NetworkThread::getInstance()->start(
            SocketAddress(argv[1], ::atoi(argv[2]))) == false) {
        return -1;
    }
    LogicThread::getInstance()->start();

    LogicThread::getInstance()->join();
    NetworkThread::getInstance()->join();

    return 0;
}
