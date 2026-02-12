#ifndef BRICKRED_SYSTEM_H
#define BRICKRED_SYSTEM_H

namespace brickred::system {

bool daemon(bool change_dir = false);
bool createPidFile(const char *file);
bool closeStdio();

using SignalHandler = void (*)(int);
SignalHandler signal(int signum, SignalHandler sighandler);

} // namespace brickred::system

#endif
