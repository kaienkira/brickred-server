#ifndef BRICKRED_SELF_PIPE_H
#define BRICKRED_SELF_PIPE_H

#include <cstddef>

#include <brickred/class_util.h>
#include <brickred/io_device.h>

namespace brickred {

class SelfPipe final : public IODevice {
public:
    SelfPipe();
    ~SelfPipe() override;

    bool open();
    void close();

    int read(char *buffer, size_t size) override;
    int write(const char *buffer, size_t size) override;
    bool setNonblock() override;
    bool setCloseOnExec() override;

private:
    BRICKRED_NONCOPYABLE(SelfPipe)

    DescriptorId fd1_;
};

} // namespace brickred

#endif
