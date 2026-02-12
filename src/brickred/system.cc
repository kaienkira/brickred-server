#include <brickred/system.h>

#include <fcntl.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>

namespace brickred::system {

bool daemon(bool change_dir)
{
    pid_t pid = ::fork();
    if (-1 == pid) {
        return false;
    } else if (pid != 0) {
        _exit(0);
    }

    if (::setsid() == -1) {
        return false;
    }

    if (change_dir) {
        if (::chdir("/") != 0) {
            return false;
        }
    }

    return true;
}

bool createPidFile(const char *file)
{
    if (file == nullptr) {
        return false;
    }

    FILE *fp = ::fopen(file, "w");
    if (fp == nullptr) {
        return false;
    }

    if (::fprintf(fp, "%d\n", ::getpid()) < 0) {
        ::fclose(fp);
        return false;
    }
    if (::fclose(fp) < 0) {
        return false;
    }

    return true;
}

bool closeStdio()
{
    int null_fd = ::open("/dev/null", O_RDWR);
    if (null_fd == -1) {
        return false;
    }
    if (::dup2(null_fd, STDIN_FILENO) == -1) {
        return false;
    }
    if (::dup2(null_fd, STDOUT_FILENO) == -1) {
        return false;
    }
    if (::dup2(null_fd, STDERR_FILENO) == -1) {
        return false;
    }
    if (null_fd > STDERR_FILENO) {
        if (::close(null_fd) == -1) {
            return false;
        }
    }

    return true;
}

SignalHandler signal(int signum, SignalHandler sighandler)
{
    struct sigaction sa_new;
    struct sigaction sa_old;

    sa_new.sa_handler = sighandler;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
#ifdef SA_INTERRUPT
    sa_new.sa_flags |= SA_INTERRUPT;
#endif

    if (sigaction(signum, &sa_new, &sa_old) != 0) {
        return SIG_ERR;
    } else {
        return sa_old.sa_handler;
    }
}

} // namespace brickred::system
