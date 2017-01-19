#include <cstdio>
#include <vector>

#include <brickred/socket_address.h>

using namespace brickred;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        ::fprintf(stderr, "usage: %s <domain>\n", argv[0]);
        return -1;
    }

    std::vector<SocketAddress> addr_list;
    SocketAddress::getAddressByDomain(argv[1], &addr_list);

    for (size_t i = 0; i < addr_list.size(); ++i) {
        ::printf("%s\n", addr_list[i].getIp().c_str());
    }

    return 0;
}
