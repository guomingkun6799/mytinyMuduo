//
// Created by MingkunGuo on 2026/10/23.
//

#include "buffer.h"

#include <errno.h>
#include <sys/uio.h>
//从fd上读取数据， Poller工作在LT模式
ssize_t buffer::readFd(int fd, int *savedErrno) {
    char extrabuf[65536] = {0}; // 栈上的内存空间 64k
    struct iovec vec[2];
    const size_t writable = writableBytes(); //这是buffer底层缓冲区剩余的科协空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf ? 2: 1);
    const ssize_t n = ::readv(fd, &vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    } else if (n <= writable) {
        writerIndex_ += n;
    } else { //extrabuf里面也写入了数据
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}
