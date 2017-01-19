#include <cstdio>
#include <iostream>

#include "test/test_util.h"
#include <brickred/log_async_sink.h>
#include <brickred/log_core.h>
#include <brickred/log_file_sink.h>

using namespace brickred;
using namespace test;

int main(void)
{
    char log_msg[] = "Hello World! Hello World! Hello World!\n";

    {
        std::cout << "***log using log_file_sink***" << std::endl;
        TestTimer timer;

        LogCore::getInstance()->registerLogger(1);
        UniquePtr<LogFileSink> sink(new LogFileSink("test1_%Y_%m_%d.log"));
        if (LogCore::getInstance()->addSink(1, sink.get(), 0) == false) {
            return -1;
        }
        sink.release();

        for (int i = 0; i < 1000000; ++i) {
            LogCore::getInstance()->log(1, 0,
                __FILE__, __LINE__, __func__, log_msg);
        }
        LogCore::getInstance()->removeLogger(1);
    }

    {
        std::cout << "***log using log_async_sink***" << std::endl;
        TestTimer timer;

        LogCore::getInstance()->registerLogger(1);
        UniquePtr<LogFileSink> file_sink(
            new LogFileSink("test2_%Y_%m_%d.log"));
        UniquePtr<LogAsyncSink> async_sink(
            new LogAsyncSink(file_sink.get()));
        file_sink.release();
        if (LogCore::getInstance()->addSink(1, async_sink.get()) == false) {
            return -1;
        }
        async_sink.release();

        for (int i = 0; i < 1000000; ++i) {
            LogCore::getInstance()->log(1, 0,
                __FILE__, __LINE__, __func__, log_msg);
        }
        LogCore::getInstance()->removeLogger(1);
    }

    {
        std::cout << "***log using fwrite directly***" << std::endl;
        TestTimer timer;

        FILE *fp = ::fopen("test.log", "a");
        if (NULL == fp) {
            return false;
        }

        for (int i = 0; i < 1000000; ++i) {
            ::fwrite(log_msg, sizeof(log_msg) - 1, 1, fp);
        }

        ::fclose(fp);
    }

    return 0;
}
