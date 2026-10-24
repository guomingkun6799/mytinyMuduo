//
// Created by MingkunGuo on 2026/10/23.
//

#pragma once

#include <vector>
#include <string>
#include <algorithm>

//网络库底层的缓冲器类型
class buffer {
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend){}

    size_t readableBytes() const {
        return writerIndex_ - readerIndex_;
    };
    size_t writableBytes() const {
        return buffer_.size() - writerIndex_;
    }
    size_t prependableBytes() const {
        return readerIndex_;
    }

    //返回缓冲区中可读数据的起始地址·
    const char* peek() const {
        return begin() + readerIndex_;
    }

    // onMessage string <- buffer
    void retrieve(size_t Len) {
        if (Len < readableBytes()) {
            readerIndex_ += Len;
        } else {
            retrieveAll();
        }
    }

    void retrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的buffer数据，转成string返回
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t Len) {
        std::string result(peek(), Len);
        retrieve(Len);
        return result;
    }

    void ensureWritableBytes(size_t Len) {
        if (writableBytes() < Len) {
            makeSpace(Len); //扩容函数
        }

    }

    //把[data, data+len]内存上的数据添加到writable缓冲区当中
    void append(const char* data, size_t Len) {
        ensureWritableBytes(Len);
        std::copy(data, data + Len, beginWrite());
        writerIndex_ += Len;
    }

    char* beginWrite() {
        return begin() + writerIndex_;
    }
    const char* beginWrite() const {
        return begin() + writerIndex_;
    }

    //从fd上读取数据
    ssize_t readFd(int fd, int* savedErrno);
    //往fd写出数据
    ssize_t writeFd(int fd, int* savedErrno);
    ssize_t writeFd(int fd, int* savedErrno);

private:

    void makeSpace(size_t Len) {
        if (writableBytes() + prependableBytes() < Len + kCheapPrepend) {
            buffer_.resize(writerIndex_ + Len);
        } else {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                begin() + writerIndex_,
                begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    char* begin() {
        return &*buffer_.begin();
    }
    const char* begin() const {
        //1. it.operator*()
        //2. 再去地址&
        return &*buffer_.begin();
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};