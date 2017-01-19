#include <brickred/tcp_service.h>

#include <cerrno>
#include <cstring>
#include <algorithm>
#include <ext/hash_map>

#include <brickred/base_logger.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_device.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/tcp_socket.h>

namespace brickred {

namespace tcp_service_impl {

class SocketIdAllocator {
public:
    SocketIdAllocator() : value_(0) {}
    ~SocketIdAllocator() {}

    int64_t getId(int fd = 0)
    {
        if (++value_ < 0) {
            value_ = 0;
        }
        return ((uint64_t)(value_) << 32) + (uint64_t)fd;
    }

private:
    int32_t value_;
};

///////////////////////////////////////////////////////////////////////////////
class TcpConnection {
public:
    struct Status {
        enum type {
            NONE,
            CONNECTING,
            CONNECTED,
            PEER_CLOSED,
            PENDING_ERROR,
        };
    };

    typedef TcpService::SendCompleteCallback SendCompleteCallback;

    explicit TcpConnection(TcpSocket *socket,
                           size_t read_buffer_init_size,
                           size_t read_buffer_expand_size,
                           size_t write_buffer_init_size,
                           size_t write_buffer_expand_size);
    ~TcpConnection() {}

    TcpSocket *getSocket() { return socket_; }
    Status::type getStatus() const { return status_; }
    int getErrorCode() const { return error_code_; }
    DynamicBuffer &getReadBuffer() { return read_buffer_; }
    DynamicBuffer &getWriteBuffer() { return write_buffer_; }

    void setStatus(Status::type status) { status_ = status; }
    void setError(int error_code);

    const SendCompleteCallback &getSendCompleteCallback() const {
        return send_complete_cb_;
    }
    void setSendCompleteCallback(
        const SendCompleteCallback &send_complete_cb)
    {
        send_complete_cb_ = send_complete_cb;
    }

private:
    BRICKRED_NONCOPYABLE(TcpConnection)

    TcpSocket *socket_;
    Status::type status_;
    int error_code_;
    DynamicBuffer read_buffer_;
    DynamicBuffer write_buffer_;
    SendCompleteCallback send_complete_cb_;
};

///////////////////////////////////////////////////////////////////////////////
TcpConnection::TcpConnection(TcpSocket *socket,
                             size_t read_buffer_init_size,
                             size_t read_buffer_expand_size,
                             size_t write_buffer_init_size,
                             size_t write_buffer_expand_size) :
    socket_(socket),
    status_(Status::NONE),
    error_code_(0),
    read_buffer_(read_buffer_init_size, read_buffer_expand_size),
    write_buffer_(write_buffer_init_size, write_buffer_expand_size)
{
}

void TcpConnection::setError(int error_code)
{
    status_ = Status::PENDING_ERROR;
    error_code_ = error_code;
}

} using namespace tcp_service_impl;

///////////////////////////////////////////////////////////////////////////////
class TcpService::Impl {
public:
    typedef TcpService::Context Context;
    typedef TcpService::SocketId SocketId;
    typedef TcpService::NewConnectionCallback NewConnectionCallback;
    typedef TcpService::RecvMessageCallback RecvMessageCallback;
    typedef TcpService::PeerCloseCallback PeerCloseCallback;
    typedef TcpService::ErrorCallback ErrorCallback;
    typedef TcpService::SendCompleteCallback SendCompleteCallback;
    typedef IOService::TimerId TimerId;
    typedef IOService::TimerCallback TimerCallback;
    typedef __gnu_cxx::hash_map<SocketId, TcpSocket *> TcpSocketMap;
    typedef __gnu_cxx::hash_map<SocketId, TcpConnection *> TcpConnectionMap;
    typedef __gnu_cxx::hash_map<SocketId, Context *> ContextMap;
    typedef __gnu_cxx::hash_map<SocketId, TimerId> SocketId_TimerId_Map;
    typedef __gnu_cxx::hash_map<TimerId, SocketId> TimerId_SocketId_Map;

    explicit Impl(TcpService *thiz, IOService &io_service);
    ~Impl();

    IOService *getIOService() const;

    SocketId listen(const SocketAddress &addr);
    SocketId shareListen(const TcpSocket &shared_socket);
    SocketId connect(const SocketAddress &addr);
    SocketId asyncConnect(const SocketAddress &addr, bool *complete,
                          int timeout_ms);

    bool isConnected(SocketId socket_id) const;

    bool getLocalAddress(SocketId socket_id, SocketAddress *addr) const;
    bool getPeerAddress(SocketId socket_id, SocketAddress *addr) const;

    bool sendMessage(SocketId socket_id, const char *buffer, size_t size,
                     const SendCompleteCallback &send_complete_cb);
    bool sendMessageThenClose(SocketId socket_id,
                              const char *buffer, size_t size);
    void broadcastMessage(const char *buffer, size_t size);
    void closeSocket(SocketId socket_id);

    Context *getContext(SocketId socket_id) const;
    bool setContext(SocketId socket_id, Context *context);

    void setNewConnectionCallback(const NewConnectionCallback &new_conn_cb);
    void setRecvMessageCallback(const RecvMessageCallback &recv_message_cb);
    void setPeerCloseCallback(const PeerCloseCallback &peer_close_cb);
    void setErrorCallback(const ErrorCallback &error_cb);

    void setRecvBufferInitSize(size_t size);
    void setRecvBufferExpandSize(size_t size);
    void setRecvBufferMaxSize(size_t size);
    void setSendBufferInitSize(size_t size);
    void setSendBufferExpandSize(size_t size);
    void setSendBufferMaxSize(size_t size);

private:
    SocketId buildListenSocket(UniquePtr<TcpSocket> &socket);
    SocketId buildConnectedSocket(UniquePtr<TcpSocket> &socket);
    SocketId buildAsyncConnectSocket(UniquePtr<TcpSocket> &socket,
                                     int timeout_ms);

    void addSocketTimer(SocketId socket_id, int timeout_ms,
                        TimerCallback timer_cb);
    void removeSocketTimer(SocketId socket_id);

    void onListenSocketRead(IODevice *io_device);
    void onAsyncConnectSocketWrite(IODevice *io_device);
    void onAsyncConnectTimeout(TimerId timer_id);
    void onSocketRead(IODevice *io_device);
    void onSocketWrite(IODevice *io_device);
    void onSocketError(IODevice *io_device);

    bool sendMessage(TcpConnection *connection,
                     const char *buffer, size_t size,
                     const SendCompleteCallback &send_complete_cb);
    void onSendMessageError(TimerId timer_id);
    void sendCompleteCloseCallback(TcpService *service, SocketId socket_id);

private:
    TcpService *thiz_;
    IOService *io_service_;

    SocketIdAllocator socket_id_allocator_;
    TcpSocketMap sockets_;
    TcpConnectionMap connections_;
    ContextMap contexts_;
    SocketId_TimerId_Map socket_to_timer_map_;
    TimerId_SocketId_Map timer_to_socket_map_;

    NewConnectionCallback new_conn_cb_;
    RecvMessageCallback recv_message_cb_;
    PeerCloseCallback peer_close_cb_;
    ErrorCallback error_cb_;

    size_t conn_read_buffer_init_size_;
    size_t conn_read_buffer_expand_size_;
    size_t conn_read_buffer_max_size_;
    size_t conn_write_buffer_init_size_;
    size_t conn_write_buffer_expand_size_;
    size_t conn_write_buffer_max_size_;
};

///////////////////////////////////////////////////////////////////////////////
TcpService::Impl::Impl(TcpService *thiz, IOService &io_service) :
    thiz_(thiz), io_service_(&io_service),
    conn_read_buffer_init_size_(0), conn_read_buffer_expand_size_(0),
    conn_read_buffer_max_size_(0),
    conn_write_buffer_init_size_(0), conn_write_buffer_expand_size_(0),
    conn_write_buffer_max_size_(0)
{
}

TcpService::Impl::~Impl()
{
    for (TcpConnectionMap::iterator iter = connections_.begin();
         iter != connections_.end(); ++iter) {
        delete iter->second;
    }
    for (TcpSocketMap::iterator iter = sockets_.begin();
         iter != sockets_.end(); ++iter) {
        delete iter->second;
    }
    for (ContextMap::iterator iter = contexts_.begin();
         iter != contexts_.end(); ++iter) {
        delete iter->second;
    }
}

IOService *TcpService::Impl::getIOService() const
{
    return io_service_;
}

TcpService::Impl::SocketId TcpService::Impl::buildListenSocket(
    UniquePtr<TcpSocket> &socket)
{
    SocketId socket_id = socket_id_allocator_.getId(socket->getDescriptor());
    socket->setId(socket_id);
    socket->setReadCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onListenSocketRead, this));
    socket->setErrorCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onSocketError, this));

    // attach io service
    socket->attachIOService(*io_service_);

    if (sockets_.find(socket_id) != sockets_.end()) {
        BASE_ERROR("socket(%lx) already in socket map", socket_id);
        return -1;
    }

    // insert into socket map
    sockets_.insert(std::make_pair(socket_id, socket.get()));
    socket.release();

    return socket_id;
}

TcpService::Impl::SocketId TcpService::Impl::buildConnectedSocket(
    UniquePtr<TcpSocket> &socket)
{
    SocketId socket_id =
        socket_id_allocator_.getId(socket->getDescriptor());
    socket->setId(socket_id);
    socket->setReadCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onSocketRead, this));
    socket->setErrorCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onSocketError, this));

    // attach io service
    socket->attachIOService(*io_service_);

    UniquePtr<TcpConnection> connection(new TcpConnection(socket.get(),
                                        conn_read_buffer_init_size_,
                                        conn_read_buffer_expand_size_,
                                        conn_write_buffer_init_size_,
                                        conn_write_buffer_expand_size_));
    connection->setStatus(TcpConnection::Status::CONNECTED);

    if (sockets_.find(socket_id) != sockets_.end()) {
        BASE_ERROR("socket(%lx) already in socket map", socket_id);
        return -1;
    }
    if (connections_.find(socket_id) != connections_.end()) {
        BASE_ERROR("socket(%lx) already in connection map", socket_id);
        return -1;
    }

    // insert into socket map
    sockets_.insert(std::make_pair(socket_id, socket.get()));
    socket.release();
    // insert into connection map
    connections_.insert(std::make_pair(socket_id, connection.get()));
    connection.release();

    return socket_id;
}

TcpService::Impl::SocketId TcpService::Impl::buildAsyncConnectSocket(
    UniquePtr<TcpSocket> &socket, int timeout_ms)
{
    SocketId socket_id =
        socket_id_allocator_.getId(socket->getDescriptor());
    socket->setId(socket_id);
    socket->setWriteCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onAsyncConnectSocketWrite, this));
    socket->setErrorCallback(BRICKRED_BIND_MEM_FUNC(
        &TcpService::Impl::onSocketError, this));

    // attach io service
    socket->attachIOService(*io_service_);

    UniquePtr<TcpConnection> connection(new TcpConnection(socket.get(),
                                        conn_read_buffer_init_size_,
                                        conn_read_buffer_expand_size_,
                                        conn_write_buffer_init_size_,
                                        conn_write_buffer_expand_size_));
    connection->setStatus(TcpConnection::Status::CONNECTING);

    if (sockets_.find(socket_id) != sockets_.end()) {
        BASE_ERROR("socket(%lx) already in socket map", socket_id);
        return -1;
    }
    if (connections_.find(socket_id) != connections_.end()) {
        BASE_ERROR("socket(%lx) already in connection map", socket_id);
        return -1;
    }

    // add timeout timer
    if (timeout_ms > 0) {
        addSocketTimer(socket_id, timeout_ms, BRICKRED_BIND_MEM_FUNC(
            &TcpService::Impl::onAsyncConnectTimeout, this));
    }

    // insert into socket map
    sockets_.insert(std::make_pair(socket_id, socket.get()));
    socket.release();
    // insert into connection map
    connections_.insert(std::make_pair(socket_id, connection.get()));
    connection.release();

    return socket_id;
}

void TcpService::Impl::addSocketTimer(SocketId socket_id,
    int timeout_ms, TimerCallback timer_cb)
{
    TimerId timer_id = io_service_->startTimer(timeout_ms, timer_cb, 1);
    socket_to_timer_map_[socket_id] = timer_id;
    timer_to_socket_map_[timer_id] = socket_id;
}

void TcpService::Impl::removeSocketTimer(SocketId socket_id)
{
    SocketId_TimerId_Map::iterator iter =
        socket_to_timer_map_.find(socket_id);
    if (socket_to_timer_map_.end() == iter) {
        return;
    }
    TimerId timer_id = iter->second;
    socket_to_timer_map_.erase(iter);
    timer_to_socket_map_.erase(timer_id);

    io_service_->stopTimer(timer_id);
}

void TcpService::Impl::onListenSocketRead(IODevice *io_device)
{
    TcpSocket *listen_socket = static_cast<TcpSocket *>(io_device);

    for (;;) {
        UniquePtr<TcpSocket> socket(new TcpSocket());

        // open connect socket
        if (listen_socket->acceptNonblock(socket.get()) == false) {
            if (EAGAIN == errno) {
                break;
            } else if (ECONNABORTED == errno) {
                // ignore
                continue;
            } else {
                BASE_ERROR("socket(%lx) accept failed: %s",
                           listen_socket->getId(), strerror(errno));
                return;
            }
        }

        SocketId socket_id = buildConnectedSocket(socket);
        if (-1 == socket_id) {
            return;
        }

        if (new_conn_cb_) {
            new_conn_cb_(thiz_, listen_socket->getId(), socket_id);
        }
    }
}

void TcpService::Impl::onAsyncConnectSocketWrite(IODevice *io_device)
{
    TcpSocket *socket = static_cast<TcpSocket *>(io_device);

    // remove timeout timer
    removeSocketTimer(socket->getId());

    TcpConnectionMap::iterator iter = connections_.find(socket->getId());
    if (connections_.end() == iter) {
        BASE_ERROR("socket(%lx) not found in connection map",
                   socket->getId());
        return;
    }
    TcpConnection *connection = iter->second;

    int socket_error = socket->getSocketError();
    if (socket_error != 0) {
        connection->setError(socket_error);
        if (error_cb_) {
            error_cb_(thiz_, socket->getId(), connection->getErrorCode());
        }
    } else {
        socket->setReadCallback(BRICKRED_BIND_MEM_FUNC(
            &TcpService::Impl::onSocketRead, this));
        socket->setWriteCallback(NullFunction());
        connection->setStatus(TcpConnection::Status::CONNECTED);
        if (new_conn_cb_) {
            new_conn_cb_(thiz_, socket->getId(), socket->getId());
        }
    }
}

void TcpService::Impl::onAsyncConnectTimeout(TimerId timer_id)
{
    TimerId_SocketId_Map::iterator iter =
        timer_to_socket_map_.find(timer_id);
    if (timer_to_socket_map_.end() == iter) {
        return;
    }
    SocketId socket_id = iter->second;
    timer_to_socket_map_.erase(iter);
    socket_to_timer_map_.erase(socket_id);

    TcpSocketMap::iterator iter2 = sockets_.find(socket_id);
    if (sockets_.end() == iter2) {
        return;
    }
    TcpConnectionMap::iterator iter3 = connections_.find(socket_id);
    if (connections_.end() == iter3) {
        return;
    }

    TcpSocket *socket = iter2->second;
    TcpConnection *connection = iter3->second;

    socket->close();
    connection->setError(ETIMEDOUT);
    if (error_cb_) {
        error_cb_(thiz_, socket->getId(), connection->getErrorCode());
    }
}

void TcpService::Impl::onSocketRead(IODevice *io_device)
{
    TcpSocket *socket = static_cast<TcpSocket *>(io_device);
    SocketId socket_id = socket->getId();

    TcpConnectionMap::iterator iter = connections_.find(socket_id);
    if (connections_.end() == iter) {
        BASE_ERROR("socket(%lx) not found in connection map",
                   socket_id);
        return;
    }
    TcpConnection *connection = iter->second;
    DynamicBuffer &read_buffer = connection->getReadBuffer();

    bool data_arrive = false;
    bool peer_close = false;

    for (;;) {
        int bytes_to_read = std::max(1, socket->readableBytes());

        // check buffer overflow
        if (conn_read_buffer_max_size_ > 0 &&
            bytes_to_read + read_buffer.readableBytes() >
                conn_read_buffer_max_size_) {
            peer_close = true;
            break;
        }

        // reserve bytes for write
        read_buffer.reserveWritableBytes(bytes_to_read);

        int ret = socket->recv(read_buffer.writeBegin(), bytes_to_read);
        if (ret > 0) {
            read_buffer.write(ret);
            data_arrive = true;
        } else if (ret < 0) {
            if (EAGAIN == errno) {
                break;
            } else {
                connection->setError(errno);
                if (error_cb_) {
                    error_cb_(thiz_, socket_id, connection->getErrorCode());
                }
                return;
            }
        } else {
            peer_close = true;
            break;
        }
    }

    if (data_arrive) {
        if (recv_message_cb_) {
            recv_message_cb_(thiz_, socket_id, &read_buffer);
        }
    }
    if (peer_close) {
        if (data_arrive) {
            // check recv message callback closed socket or not
            if (connections_.find(socket_id) == connections_.end()) {
                return;
            }
        }
        connection->setStatus(TcpConnection::Status::PEER_CLOSED);
        if (peer_close_cb_) {
            peer_close_cb_(thiz_, socket_id);
        }
    }
}

void TcpService::Impl::onSocketWrite(IODevice *io_device)
{
    TcpSocket *socket = static_cast<TcpSocket *>(io_device);

    TcpConnectionMap::iterator iter = connections_.find(socket->getId());
    if (connections_.end() == iter) {
        BASE_ERROR("socket(%lx) not found in connection map",
                   socket->getId());
        return;
    }
    TcpConnection *connection = iter->second;
    DynamicBuffer &write_buffer = connection->getWriteBuffer();

    int write_size = socket->send(write_buffer.readBegin(),
                                  write_buffer.readableBytes());
    if (write_size < 0) {
        if (errno != EAGAIN) {
            connection->setError(errno);
            if (error_cb_) {
                error_cb_(thiz_, socket->getId(), connection->getErrorCode());
            }
            return;
        }
    } else {
        write_buffer.read(write_size);
        if (write_buffer.readableBytes() == 0) {
            socket->setWriteCallback(NullFunction());
            SendCompleteCallback send_complete_cb =
                connection->getSendCompleteCallback();
            connection->setSendCompleteCallback(NullFunction());
            if (send_complete_cb) {
                send_complete_cb(thiz_, socket->getId());
            }
        }
    }
}

void TcpService::Impl::onSocketError(IODevice *io_device)
{
    TcpSocket *socket = static_cast<TcpSocket *>(io_device);

    TcpConnectionMap::iterator iter = connections_.find(socket->getId());
    if (connections_.end() == iter) {
        BASE_ERROR("socket(%lx) not found in connection map",
                   socket->getId());
        return;
    }
    TcpConnection *connection = iter->second;

    int socket_error = socket->getSocketError();
    if (0 == socket_error) {
        socket_error = errno;
    }
    connection->setError(socket_error);
    if (error_cb_) {
        error_cb_(thiz_, socket->getId(), connection->getErrorCode());
    }
}

TcpService::Impl::SocketId TcpService::Impl::listen(const SocketAddress &addr)
{
    UniquePtr<TcpSocket> socket(new TcpSocket());

    // open listen socket
    if (socket->passiveOpenNonblock(addr) == false) {
        return -1;
    }

    return buildListenSocket(socket);
}

TcpService::Impl::SocketId TcpService::Impl::shareListen(
    const TcpSocket &shared_socket)
{
    UniquePtr<TcpSocket> socket(new TcpSocket());
    if (socket->dupDescriptor(shared_socket.getDescriptor()) == false) {
        return -1;
    }

    return buildListenSocket(socket);
}

TcpService::Impl::SocketId TcpService::Impl::connect(const SocketAddress &addr)
{
    UniquePtr<TcpSocket> socket(new TcpSocket());

    // open connect socket
    if (socket->activeOpenNonblock(addr) == false) {
        return -1;
    }

    return buildConnectedSocket(socket);
}

TcpService::Impl::SocketId TcpService::Impl::asyncConnect(
    const SocketAddress &addr, bool *complete, int timeout_ms)
{
    UniquePtr<TcpSocket> socket(new TcpSocket());

    // open connect socket
    if (socket->open(addr.getProtocol()) == false) {
        return -1;
    }
    if (socket->setReuseAddr() == false ||
        socket->setTcpNoDelay() == false ||
        socket->setNonblock() == false) {
        return -1;
    }

    if (socket->connect(addr) == true) {
        // connection is completed immediately
        SocketId socket_id = buildConnectedSocket(socket);
        if (-1 == socket_id) {
            return - 1;
        }
        if (complete != NULL) {
            *complete = true;
        }

        return socket_id;

    } else if (EINPROGRESS == errno) {
        // connection cannot be completed immediately
        SocketId socket_id = buildAsyncConnectSocket(socket, timeout_ms);
        if (-1 == socket_id) {
            return - 1;
        }
        if (complete != NULL) {
            *complete = false;
        }

        return socket_id;

    } else {
        return -1;
    }

    return 0;
}

bool TcpService::Impl::isConnected(SocketId socket_id) const
{
    TcpConnectionMap::const_iterator iter = connections_.find(socket_id);
    if (connections_.end() == iter) {
        return false;
    }
    TcpConnection *connection = iter->second;

    return connection->getStatus() == TcpConnection::Status::CONNECTED;
}

bool TcpService::Impl::getLocalAddress(SocketId socket_id,
                                       SocketAddress *addr) const
{
    TcpSocketMap::const_iterator iter = sockets_.find(socket_id);
    if (iter == sockets_.end()) {
        return false;
    }
    TcpSocket *socket = iter->second;

    return socket->getLocalAddress(addr);
}

bool TcpService::Impl::getPeerAddress(SocketId socket_id,
                                      SocketAddress *addr) const
{
    TcpSocketMap::const_iterator iter = sockets_.find(socket_id);
    if (iter == sockets_.end()) {
        return false;
    }
    TcpSocket *socket = iter->second;

    return socket->getPeerAddress(addr);
}

bool TcpService::Impl::sendMessage(SocketId socket_id,
    const char *buffer, size_t size,
    const SendCompleteCallback &send_complete_cb)
{
    TcpConnectionMap::iterator iter = connections_.find(socket_id);
    if (connections_.end() == iter) {
        return false;
    }
    TcpConnection *connection = iter->second;

    return sendMessage(connection, buffer, size, send_complete_cb);
}

void TcpService::Impl::broadcastMessage(const char *buffer, size_t size)
{
    for (TcpConnectionMap::iterator iter = connections_.begin();
         iter != connections_.end(); ++iter) {
        TcpConnection *connection = iter->second;
        sendMessage(connection, buffer, size, NullFunction());
    }
}

bool TcpService::Impl::sendMessage(TcpConnection *connection,
    const char *buffer, size_t size,
    const SendCompleteCallback &send_complete_cb)
{
    if (connection->getStatus() != TcpConnection::Status::CONNECTED) {
        return false;
    }

    TcpSocket *socket = connection->getSocket();
    DynamicBuffer &write_buffer = connection->getWriteBuffer();
    size_t remain_size = size;

    // check write buffer is empty
    if (write_buffer.readableBytes() == 0) {
        // send directly
        int write_size = socket->send(buffer, size);
        if (write_size < 0) {
            if (errno != EAGAIN) {
                connection->setError(errno);
                addSocketTimer(socket->getId(), 0, BRICKRED_BIND_MEM_FUNC(
                    &TcpService::Impl::onSendMessageError, this));
                return false;
            }
        } else {
            remain_size -= write_size;
        }
    }

    if (remain_size > 0) {
        // check buffer overflow
        if (conn_write_buffer_max_size_ > 0 &&
            remain_size + write_buffer.readableBytes() >
                conn_write_buffer_max_size_) {
            connection->setError(ENOBUFS);
            addSocketTimer(socket->getId(), 0, BRICKRED_BIND_MEM_FUNC(
                &TcpService::Impl::onSendMessageError, this));
            return false;
        }

        // write to write buffer
        write_buffer.reserveWritableBytes(remain_size);
        ::memcpy(write_buffer.writeBegin(), buffer, remain_size);
        write_buffer.write(remain_size);
        // set send complete callback
        connection->setSendCompleteCallback(send_complete_cb);
        // set writeable callback
        socket->setWriteCallback(BRICKRED_BIND_MEM_FUNC(
            &TcpService::Impl::onSocketWrite, this));
    } else {
        if (send_complete_cb) {
            send_complete_cb(thiz_, socket->getId());
        }
    }

    return true;
}

void TcpService::Impl::onSendMessageError(TimerId timer_id)
{
    TimerId_SocketId_Map::iterator iter =
        timer_to_socket_map_.find(timer_id);
    if (timer_to_socket_map_.end() == iter) {
        return;
    }
    SocketId socket_id = iter->second;
    timer_to_socket_map_.erase(iter);
    socket_to_timer_map_.erase(socket_id);

    TcpSocketMap::iterator iter2 = sockets_.find(socket_id);
    if (sockets_.end() == iter2) {
        return;
    }
    TcpConnectionMap::iterator iter3 = connections_.find(socket_id);
    if (connections_.end() == iter3) {
        return;
    }

    TcpSocket *socket = iter2->second;
    TcpConnection *connection = iter3->second;

    if (error_cb_) {
        error_cb_(thiz_, socket->getId(), connection->getErrorCode());
    }
}

void TcpService::Impl::sendCompleteCloseCallback(TcpService *service,
    SocketId socket_id)
{
    service->closeSocket(socket_id);
}

bool TcpService::Impl::sendMessageThenClose(SocketId socket_id,
    const char *buffer, size_t size)
{
    return sendMessage(socket_id, buffer, size, BRICKRED_BIND_MEM_FUNC(
                       &TcpService::Impl::sendCompleteCloseCallback, this));
}

void TcpService::Impl::closeSocket(SocketId socket_id)
{
    {
        TcpConnectionMap::iterator iter = connections_.find(socket_id);
        if (iter != connections_.end()) {
            delete iter->second;
            connections_.erase(iter);
        }
    }
    {
        TcpSocketMap::iterator iter = sockets_.find(socket_id);
        if (iter != sockets_.end()) {
            delete iter->second;
            sockets_.erase(iter);
        }
    }
    {
        ContextMap::iterator iter = contexts_.find(socket_id);
        if (iter != contexts_.end()) {
            delete iter->second;
            contexts_.erase(iter);
        }
    }
}

TcpService::Impl::Context *TcpService::Impl::getContext(
    SocketId socket_id) const
{
    ContextMap::const_iterator iter = contexts_.find(socket_id);
    if (iter != contexts_.end()) {
        return iter->second;
    }

    return NULL;
}

bool TcpService::Impl::setContext(SocketId socket_id, Context *context)
{
    {
        TcpSocketMap::iterator iter = sockets_.find(socket_id);
        if (sockets_.end() == iter) {
            return false;
        }
    }

    ContextMap::iterator iter = contexts_.find(socket_id);
    if (iter != contexts_.end()) {
        delete iter->second;
        contexts_.erase(iter);
    }
    contexts_.insert(std::make_pair(socket_id, context));

    return true;
}

void TcpService::Impl::setNewConnectionCallback(
    const NewConnectionCallback &new_conn_cb)
{
    new_conn_cb_ = new_conn_cb;
}

void TcpService::Impl::setRecvMessageCallback(
    const RecvMessageCallback &recv_message_cb)
{
    recv_message_cb_ = recv_message_cb;
}

void TcpService::Impl::setErrorCallback(
    const ErrorCallback &error_cb)
{
    error_cb_ = error_cb;
}

void TcpService::Impl::setPeerCloseCallback(
    const PeerCloseCallback &peer_close_cb)
{
    peer_close_cb_ = peer_close_cb;
}

void TcpService::Impl::setRecvBufferInitSize(size_t size)
{
    conn_read_buffer_init_size_ = size;
}

void TcpService::Impl::setRecvBufferExpandSize(size_t size)
{
    conn_read_buffer_expand_size_ = size;
}

void TcpService::Impl::setRecvBufferMaxSize(size_t size)
{
    conn_read_buffer_max_size_ = size;
}

void TcpService::Impl::setSendBufferInitSize(size_t size)
{
    conn_write_buffer_init_size_ = size;
}

void TcpService::Impl::setSendBufferExpandSize(size_t size)
{
    conn_write_buffer_expand_size_ = size;
}

void TcpService::Impl::setSendBufferMaxSize(size_t size)
{
    conn_write_buffer_max_size_ = size;
}

///////////////////////////////////////////////////////////////////////////////
TcpService::Context::~Context()
{
}

///////////////////////////////////////////////////////////////////////////////
TcpService::TcpService(IOService &io_service) :
    pimpl_(new Impl(this, io_service))
{
    setRecvBufferInitSize();
    setRecvBufferExpandSize();
    setRecvBufferMaxSize();
    setSendBufferInitSize();
    setSendBufferExpandSize();
    setSendBufferMaxSize();
}

TcpService::~TcpService()
{
}

IOService *TcpService::getIOService() const
{
    return pimpl_->getIOService();
}

TcpService::SocketId TcpService::listen(const SocketAddress &addr)
{
    return pimpl_->listen(addr);
}

TcpService::SocketId TcpService::shareListen(const TcpSocket &shared_socket)
{
    return pimpl_->shareListen(shared_socket);
}

TcpService::SocketId TcpService::connect(const SocketAddress &addr)
{
    return pimpl_->connect(addr);
}

TcpService::SocketId TcpService::asyncConnect(
    const SocketAddress &addr, bool *complete, int timeout_ms)
{
    return pimpl_->asyncConnect(addr, complete, timeout_ms);
}

bool TcpService::isConnected(SocketId socket_id) const
{
    return pimpl_->isConnected(socket_id);
}

bool TcpService::getLocalAddress(SocketId socket_id, SocketAddress *addr) const
{
    return pimpl_->getLocalAddress(socket_id, addr);
}

bool TcpService::getPeerAddress(SocketId socket_id, SocketAddress *addr) const
{
    return pimpl_->getPeerAddress(socket_id, addr);
}

bool TcpService::sendMessage(SocketId socket_id, const char *buffer,
    size_t size, const SendCompleteCallback &send_complete_cb)
{
    return pimpl_->sendMessage(socket_id, buffer, size, send_complete_cb);
}

bool TcpService::sendMessageThenClose(SocketId socket_id,
    const char *buffer, size_t size)
{
    return pimpl_->sendMessageThenClose(socket_id, buffer, size);
}

void TcpService::broadcastMessage(const char *buffer, size_t size)
{
    return pimpl_->broadcastMessage(buffer, size);
}

void TcpService::closeSocket(SocketId socket_id)
{
    pimpl_->closeSocket(socket_id);
}

TcpService::Context *TcpService::getContext(SocketId socket_id) const
{
    return pimpl_->getContext(socket_id);
}

bool TcpService::setContext(SocketId socket_id, Context *context)
{
    return pimpl_->setContext(socket_id, context);
}

void TcpService::setNewConnectionCallback(
    const NewConnectionCallback &new_conn_cb)
{
    pimpl_->setNewConnectionCallback(new_conn_cb);
}

void TcpService::setRecvMessageCallback(
    const RecvMessageCallback &recv_message_cb)
{
    pimpl_->setRecvMessageCallback(recv_message_cb);
}

void TcpService::setErrorCallback(
    const ErrorCallback &error_cb)
{
    pimpl_->setErrorCallback(error_cb);
}

void TcpService::setPeerCloseCallback(
    const PeerCloseCallback &peer_close_cb)
{
    pimpl_->setPeerCloseCallback(peer_close_cb);
}

void TcpService::setRecvBufferInitSize(size_t size)
{
    pimpl_->setRecvBufferInitSize(size);
}

void TcpService::setRecvBufferExpandSize(size_t size)
{
    pimpl_->setRecvBufferExpandSize(size);
}

void TcpService::setRecvBufferMaxSize(size_t size)
{
    pimpl_->setRecvBufferMaxSize(size);
}

void TcpService::setSendBufferInitSize(size_t size)
{
    pimpl_->setSendBufferInitSize(size);
}

void TcpService::setSendBufferExpandSize(size_t size)
{
    pimpl_->setSendBufferExpandSize(size);
}

void TcpService::setSendBufferMaxSize(size_t size)
{
    pimpl_->setSendBufferMaxSize(size);
}

} // namespace brickred
