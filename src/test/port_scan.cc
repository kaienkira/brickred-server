#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>
#include <utility>

#include <brickred/command_line_option.h>
#include <brickred/dynamic_buffer.h>
#include <brickred/io_service.h>
#include <brickred/socket_address.h>
#include <brickred/string_util.h>
#include <brickred/tcp_service.h>

using namespace brickred;

class SocketAddressRange {
public:
    typedef std::pair<uint32_t, uint32_t> IpRange;
    typedef std::pair<uint16_t, uint16_t> PortRange;

    SocketAddressRange() :
        ip_range_index_(0), port_range_index_(0),
        ip_(0), port_(0), finished_(false)
    {
    }

    bool init(const std::string &ip_range,
              const std::string &port_range)
    {
        if (parseIpRange(ip_range) == false) {
            return false;
        }
        if (parsePortRange(port_range) == false) {
            return false;
        }
        finished_ = false;

        return true;
    }

    const SocketAddress *getNextAddress()
    {
        if (finished_)
        {
            return NULL;
        }

        addr_.setAddress(SocketAddress::ipV4BinToText(ip_), port_);

        addPort();

        return &addr_;
    }

private:
    bool parseIpRange(const std::string &ip_range)
    {
        std::vector<std::string> split_1;
        string_util::split(ip_range.c_str(), ",", &split_1);

        for (size_t i = 0; i < split_1.size(); ++i) {
            std::vector<std::string> split_2;
            string_util::split(split_1[i].c_str(), "-", &split_2);

            uint32_t ip_start;
            uint32_t ip_end;

            if (split_2.size() == 1) {
                if (SocketAddress::ipV4TextToBin(
                        split_2[0], &ip_start) == false) {
                    return false;
                }
                ip_end = ip_start;

            } else if (split_2.size() == 2) {
                if (SocketAddress::ipV4TextToBin(
                        split_2[0], &ip_start) == false) {
                    return false;
                }
                if (SocketAddress::ipV4TextToBin(
                        split_2[1], &ip_end) == false) {
                    return false;
                }

            } else {
                return false;
            }

            if (ip_start > ip_end) {
                return false;
            }

            ip_ranges_.push_back(std::make_pair(ip_start, ip_end));
        }

        if (ip_ranges_.empty()) {
            return false;
        }
        ip_range_index_ = 0;
        ip_ = ip_ranges_[0].first;

        return true;
    }

    bool parsePortRange(const std::string &port_range)
    {
        std::vector<std::string> split_1;
        string_util::split(port_range.c_str(), ",", &split_1);

        for (size_t i = 0; i < split_1.size(); ++i) {
            std::vector<std::string> split_2;
            string_util::split(split_1[i].c_str(), "-", &split_2);

            uint16_t port_start;
            uint16_t port_end;

            if (split_2.size() == 1) {
                port_start = port_end = ::atoi(split_2[0].c_str());

            } else if (split_2.size() == 2) {
                port_start = ::atoi(split_2[0].c_str());
                port_end = ::atoi(split_2[1].c_str());

            } else {
                return false;
            }

            if (port_start > port_end) {
                return false;
            }

            port_ranges_.push_back(std::make_pair(port_start, port_end));
        }

        if (port_ranges_.empty()) {
            return false;
        }
        port_range_index_ = 0;
        port_ = port_ranges_[0].first;

        return true;
    }

    void addPort()
    {
        // avoid overflow
        if (port_ + 1 > port_ranges_[port_range_index_].second) {
            addPortRange();
        } else {
            ++port_;
        }
    }

    void addPortRange()
    {
        if (++port_range_index_ >= port_ranges_.size()) {
            addIp();
        }

        if (!finished_) {
            port_ = port_ranges_[port_range_index_].first;
        }
    }

    void addIp()
    {
        // avoid overflow
        if (ip_ + 1 > ip_ranges_[ip_range_index_].second) {
            addIpRange();
        } else {
            ++ip_;
        }
        
        if (!finished_) {
            port_range_index_ = 0;
        }
    }

    void addIpRange()
    {
        if (++ip_range_index_ >= ip_ranges_.size()) {
            finished_ = true;
        } else {
            ip_ = ip_ranges_[ip_range_index_].first;
        }
    }

private:
    std::vector<IpRange> ip_ranges_;
    std::vector<PortRange> port_ranges_;
    SocketAddress addr_;
    size_t ip_range_index_;
    size_t port_range_index_;
    uint32_t ip_;
    uint16_t port_;
    bool finished_;
};

class PortScanner {
public:
    typedef std::vector<uint16_t> PortVector;
    typedef std::map<std::string, PortVector> OpenPortsMap;

    PortScanner() : tcp_service_(io_service_),
        max_conn_count_(0), timeout_(0), active_conn_count_(0),
        started_scan_count_(0), finished_scan_count_(0)
    {
        tcp_service_.setNewConnectionCallback(BRICKRED_BIND_MEM_FUNC(
            &PortScanner::onNewConnection, this));
        tcp_service_.setPeerCloseCallback(BRICKRED_BIND_MEM_FUNC(
            &PortScanner::onPeerClose, this));
        tcp_service_.setErrorCallback(BRICKRED_BIND_MEM_FUNC(
            &PortScanner::onError, this));
    }

    void scan(const SocketAddressRange &addr_range,
              int max_conn_count, int timeout)
    {
        addr_range_ = addr_range;
        open_ports_.clear();
        max_conn_count_ = max_conn_count;
        timeout_ = timeout;
        active_conn_count_ = 0;
        started_scan_count_ = 0;
        finished_scan_count_ = 0;

        tryConnect();
        io_service_.loop();
    }

    void report() const
    {
        for (OpenPortsMap::const_iterator iter = open_ports_.begin();
             iter != open_ports_.end(); ++iter) {
            ::printf("%s: ", iter->first.c_str());
            const PortVector &ports = iter->second;
            for (size_t i = 0; i < ports.size(); ++i) {
                ::printf("%d ", ports[i]);
            }
            ::printf("\n");
        }
    }

private:
    bool allocateConnection()
    {
        if (active_conn_count_ >= max_conn_count_) {
            return false;
        }

        ++active_conn_count_;
        ++started_scan_count_;

        return true;
    }
    
    void freeConnection()
    {
        ++finished_scan_count_;
        --active_conn_count_;
    }

    void addSuccuss(const SocketAddress &addr)
    {
        open_ports_[addr.getIp()].push_back(addr.getPort());
        freeConnection();
    }

    void addFailed()
    {
        freeConnection();
    }

    void checkFinished()
    {
        if (finished_scan_count_ == started_scan_count_) {
            io_service_.quit();
        }
    }

    void tryConnect()
    {
        for (;;) {
            if (allocateConnection() == false) {
                return;
            }
            const SocketAddress *addr = addr_range_.getNextAddress();
            if (NULL == addr) {
                freeConnection();
                checkFinished();
                return;
            }

            bool complete = false;
            if (tcp_service_.asyncConnect(*addr,
                                          &complete, timeout_) >= 0) {
                if (complete) {
                    addSuccuss(*addr);
                }
            } else {
                addFailed();
            }
        }
    }

    void onNewConnection(TcpService *service,
                         TcpService::SocketId from_socket_id,
                         TcpService::SocketId socket_id)
    {
        SocketAddress peer_addr;
        if (service->getPeerAddress(socket_id, &peer_addr) == true) {
            addSuccuss(peer_addr);
        }
        service->closeSocket(socket_id);
        tryConnect();
    }

    void onPeerClose(TcpService *service,
                     TcpService::SocketId socket_id)
    {
        service->closeSocket(socket_id);
    }

    void onError(TcpService *service,
                 TcpService::SocketId socket_id,
                 int error)
    {
        addFailed();
        service->closeSocket(socket_id);
        tryConnect();
    }

private:
    IOService io_service_;
    TcpService tcp_service_;
    SocketAddressRange addr_range_;
    OpenPortsMap open_ports_;
    int max_conn_count_;
    int timeout_;
    int active_conn_count_;
    int started_scan_count_;
    int finished_scan_count_;
};

static void printUsage(const char *progname)
{
    ::fprintf(stderr, "usage: %s <ipv4_addr_range> \n"
              "[-p <port_range>] [-n <max_conn_count>] [-t <timeout>]\n",
              progname);
}

int main(int argc, char *argv[])
{
    std::string ip_range;
    std::string port_range = "1-65535";
    int max_conn_count = 50;
    int timeout = 1000;

    CommandLineOption options;
    options.addOption("p", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("n", CommandLineOption::ParameterType::REQUIRED);
    options.addOption("t", CommandLineOption::ParameterType::REQUIRED);

    if (options.parse(argc, argv) == false) {
        printUsage(argv[0]);
        return -1;
    }
    if (options.hasOption("p")) {
        port_range = options.getParameter("p");
    }
    if (options.hasOption("n")) {
        max_conn_count = ::atoi(options.getParameter("n").c_str());
    }
    if (options.hasOption("t")) {
        timeout = ::atoi(options.getParameter("t").c_str());
    }
    if (options.getLeftArguments().size() != 1) {
        printUsage(argv[0]);
        return -1;
    }
    ip_range = options.getLeftArguments()[0];

    SocketAddressRange addr_range;
    if (addr_range.init(ip_range, port_range) == false) {
        printUsage(argv[0]);
        return -1;
    }

    PortScanner scanner;
    scanner.scan(addr_range, max_conn_count, timeout);
    scanner.report();

    return 0;
}
