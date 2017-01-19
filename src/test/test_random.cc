#include <cstddef>
#include <iostream>

#include <brickred/random.h>

using namespace brickred;

int main(void)
{
    {
        Random r(0);

        std::cout << "r(0).nextInt()" << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::cout << r.nextInt() << std::endl;
        }
        std::cout << std::endl;

        std::cout << "r(0).nextDouble()" << std::endl;
        for (int i = 0; i < 10; ++i) {
            std::cout << r.nextDouble() << std::endl;
        }
        std::cout << std::endl;
    }
    {
        std::cout << "r(1..0).nextInt()" << std::endl;
        uint32_t key[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
        Random r(key, 10);
        for (int i = 0; i < 10; ++i) {
            std::cout << r.nextInt() << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;

    {
        std::cout << "r().nextInt()" << std::endl;
        Random r;
        for (int i = 0; i < 10; ++i) {
            std::cout << r.nextInt() << std::endl;
        }
        std::cout << std::endl;

        std::cout << "r().nextInt(2)" << std::endl;
        for (int i = 0; i < 50; ++i) {
            std::cout << r.nextInt(2) << std::endl;
        }
        std::cout << std::endl;
    }

    return 0;
}
