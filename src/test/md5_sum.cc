#include <cstdio>

#include <brickred/codec/md5.h>

using namespace brickred;

int main(int argc, char *argv[])
{
    FILE *fp = NULL;

    if (1 == argc) {
        fp = stdin;
    } else if (2 == argc) {
        fp = ::fopen(argv[1], "rb");
        if (NULL == fp) {
            ::fprintf(stderr, "can not open file %s\n", argv[1]);
            return -1;
        }
    } else {
        ::fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return -1;
    }

    codec::Md5 ctx;
    char buffer[512];

    for (;;) {
        size_t count = ::fread(buffer, 1, sizeof(buffer), fp);
        if (count > 0) {
            ctx.update(buffer, count);
        }
        if (count < sizeof(buffer)) {
            break;
        }
    }

    ::fclose(fp);

    ::printf("%s\n", ctx.digest().c_str());

    return 0;
}
