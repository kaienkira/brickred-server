#include <brickred/codec/url.h>

#include <algorithm>
#include <vector>

namespace brickred::codec {

static const char s_hex_to_letter[] = "0123456789ABCDEF";

static bool isUnreservedChar(char in)
{
    switch (in) {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '-': case '.': case '_': case '~':
        return true;

    default:
        break;
    }

    return false;
}

static int letterToHex(char in)
{
    switch (in) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    case 'a':
    case 'A':
        return 10;
    case 'b':
    case 'B':
        return 11;
    case 'c':
    case 'C':
        return 12;
    case 'd':
    case 'D':
        return 13;
    case 'e':
    case 'E':
        return 14;
    case 'f':
    case 'F':
        return 15;

    default:
        break;
    }

    return -1;
}

int urlEncode(const char *in, size_t in_size,
              char *out, size_t out_size)
{
    const char *out_start = out;
    const char *out_end = out + out_size;
    const char *in_end = in + in_size;

    for (const char *p = in; p != in_end; ++p) {
        if (isUnreservedChar(*p)) {
            if (out_end - out < 1) {
                return -1;
            }
            *out++ = *p;

        } else {
            if (out_end - out < 3) {
                return -1;
            }
            *out++ = '%';
            *out++ = s_hex_to_letter[(unsigned char)(*p) >> 4];
            *out++ = s_hex_to_letter[(unsigned char)(*p) & 0x0f];
        }
    }

    return out - out_start;
}

std::string urlEncode(const std::string &str)
{
    return urlEncode(str.c_str(), str.size());
}

std::string urlEncode(const char *buffer, size_t size)
{
    std::vector<char> output(std::max((size_t)1, size * 3));
    int count = urlEncode(buffer, size, &output[0], output.size());
    if (count <= 0) {
        return std::string();
    } else {
        return std::string(&output[0], count);
    }
}

int urlDecode(const char *in, size_t in_size,
              char *out, size_t out_size)
{
    const char *out_start = out;
    const char *out_end = out + out_size;
    const char *in_end = in + in_size;

    for (; in < in_end;) {
        if ('%' == *in) {
            if (in_end - in < 3 || out_end - out < 1) {
                return -1;
            }

            ++in;
            int high = letterToHex(*in++);
            int low = letterToHex(*in++);
            if (high < 0 || low < 0) {
                return -1;
            }
            *out++ = (char)(high * 0x10 + low);

        } else if (isUnreservedChar(*in)) {
            if (out_end - out < 1) {
                return -1;
            }
            *out++ = *in++;

        } else {
            return -1;
        }
    }

    return out - out_start;
}

std::string urlDecode(const std::string &str)
{
    return urlDecode(str.c_str(), str.size());
}

std::string urlDecode(const char *buffer, size_t size)
{
    std::vector<char> output(size);
    int count = urlDecode(buffer, size, &output[0], output.size());
    if (count <= 0) {
        return std::string();
    } else {
        return std::string(&output[0], count);
    }
}

} // namespace brickred::codec
