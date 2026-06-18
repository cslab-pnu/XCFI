#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags, ...)
{
    (void)pathname;
    (void)flags;
    errno = ENOSYS;
    return -1;
}

int close(int fd)
{
    (void)fd;
    errno = ENOSYS;
    return -1;
}

ssize_t read(int fd, void *buf, size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

ssize_t write(int fd, const void *buf, const size_t count)
{
    (void)fd;
    (void)buf;
    (void)count;
    errno = ENOSYS;
    return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    errno = ENOSYS;
    return (off_t)-1;
}

int fstat(int fd, struct stat *st)
{
    (void)fd;
    (void)st;
    errno = ENOSYS;
    return -1;
}

int isatty(int fd)
{
    (void)fd;
    return 0;
}
