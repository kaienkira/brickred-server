#include <brickred/system.h>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <csignal>
#include <cstdio>

namespace brickred {
namespace system {

bool daemon(bool change_dir, bool close_stdio)
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

    if (close_stdio) {
        if (::freopen("/dev/null", "r", stdin) == NULL) {
            return false;
        }
        if (::freopen("/dev/null", "w", stdout) == NULL) {
            return false;
        }
        if (::freopen("/dev/null", "w", stderr) == NULL) {
            return false;
        }
    }

    return true;
}

bool createPidFile(const char *file)
{
    FILE *fp = ::fopen(file, "w");
    if (NULL == fp) {
        return false;
    }

    ::fprintf(fp, "%d\n", ::getpid());
    ::fclose(fp);

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

    if (sigaction(signum, &sa_new, &sa_old) != 0)
    {
        return SIG_ERR;
    }
    else
    {
        return sa_old.sa_handler;
    }
}

} // namespace system
} // namespace brickred
