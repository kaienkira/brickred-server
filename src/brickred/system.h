#ifndef BRICKRED_SYSTEM_H
#define BRICKRED_SYSTEM_H

namespace brickred {
namespace system {

bool daemon(bool change_dir = false, bool close_stdio = false);
bool createPidFile(const char *file);

typedef void (*SignalHandler)(int);
SignalHandler signal(int signum, SignalHandler sighandler);

} // namespace system
} // namespace brickred

#endif
