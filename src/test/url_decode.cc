#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <brickred/codec/url.h>

using namespace brickred;

int main(int argc, char *argv[])
{
    std::stringstream ss;

    if (1 == argc) {
        ss << std::cin.rdbuf();

    } else if (2 == argc) {
        std::ifstream fs(argv[1]);
        if (fs.is_open() == false) {
            ::fprintf(stderr, "can not open file %s\n", argv[1]);
            return -1;
        }
        ss << fs.rdbuf();

    } else {
        ::fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return -1;
    }

    std::cout << codec::urlDecode(ss.str());

    return 0;
}
