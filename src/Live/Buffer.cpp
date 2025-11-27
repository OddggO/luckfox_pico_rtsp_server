#include "Buffer.h"
#include "Log.h"
#include "SocketOps.h"
#include "string.h"
#include <cstdarg>

const int Buffer::initialBufferSize = 1024;
const char* Buffer::kCRLF = "\r\n";
// bufferSize的默认值为initialBufferSize
Buffer::Buffer(int bufferSize): mBufferSize(bufferSize), mReadIndex(0), mWriteIndex(0)
{
    LOGI("Buffer::initialBufferSize = %d\n", bufferSize);
    LOGI("Buffer::kCRLF = %s\n", kCRLF);
    mBuffer = new char[mBufferSize];
}

Buffer::~Buffer()
{
    mReadIndex = mWriteIndex = 0;
    delete mBuffer;
}

void Buffer::ensureWritableSpace(int len)
{
    if (writeableSize() < len) {
        makeSpace(len);
    }
    assert(writeableSize() >= len);
}

void Buffer::makeSpace(int len)
{
    // int prependable = prependableSize();
    if (prependableSize() + writeableSize() >= len) {
        // 移动已读缓冲区
        int readable = readableSize();
        std::copy(beginRead(), beginWrite(), beginBuffer());
        mReadIndex = 0;
        // mWriteIndex -= prependable; // 和下面的公式在数学上等价
        mWriteIndex = mReadIndex + readable; // = 0 + readable
        assert(readableSize() == readable);
    } else {
        mBufferSize = mWriteIndex + len;
        // if (!std::realloc(mBuffer, mBufferSize)) {
        //     LOGE("Buffer::makeSpace realloc failed! mBufferSize = %d\n", mBufferSize);
        //     exit(-1);
        // }
        // 需要将realloc的结果赋值回mBuffer，以防指针变化。
        // 否则，如果realloc不是在原地扩展内存的话，mBuffer仍指向旧地址，会导致内存泄漏和未定义行为。
        mBuffer = (char*)std::realloc(mBuffer, mBufferSize); // 如果realloc是分配新内存，会释放旧内存，所以不用手动delete
        if (!mBuffer) {
            LOGE("Buffer::makeSpace realloc failed! mBufferSize = %d\n", mBufferSize);
            exit(-1);
        }
    }
}

int Buffer::append(const char* data, int len)
{
    ensureWritableSpace(len);
    assert(writeableSize() >= len);
    std::copy(data, data + len, beginWrite()); // 拷贝数据
    mWriteIndex += len;
    return len;
}

int Buffer::append(const void* data, int len)
{
    return append((const char*)data, len);
}

// int Buffer::read(int clientFd)
// {
//     if (writeableSize() < 4096) {
//         ensureWritableSpace(4096);
//     }
//     int n = sockets::read(clientFd, beginWrite(), writeableSize());
//     if (n < 0) {
//         LOGE("read error, clientFd = %d\n, return %d\n",clientFd, n);
//         return -1;
//     }
//     mWriteIndex += n;
//     assert(mWriteIndex <= mBufferSize);
//     if (n >= 4096) {
//         ensureWritableSpace(65536);
//     }
//     return n;
// }

int Buffer::read(int clientFd)
{
    char extrabuf[65536];
    int n = sockets::read(clientFd, extrabuf, sizeof extrabuf);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // 非阻塞读，没有数据可读
        }
        LOGE("read error, clientFd = %d\n, return %d\n",clientFd, n);
        // exit(-1);
        return -1;
    }
    if (n > writeableSize()) {
        ensureWritableSpace(n);
    }
    std::copy(extrabuf, extrabuf + n, beginWrite());
    mWriteIndex += n;
    assert(mWriteIndex <= mBufferSize);
    return n;
}

// int Buffer::read(const char* str, int size)
// {
//     if (!str || size <= 0) {
//         return false;
//     }
//     if (strlen(str) != size) {
//         LOGI("strlen(str) != size, the former is %d and the last is %d", (int)strlen(str), size);
//         size = strlen(str);
//     }
//     ensureWritableSpace(size);
//     std::copy(str, str + size, beginWrite());
//     mWriteIndex += size;
//     return size;
// }

int Buffer::appendFormatted(const char* fmt, ...)
{
    if (!fmt) {
        return -1;
    }
    // 计算格式化字符串的长度
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);
    if (needed < 0) {
        return -1;
    }
    ensureWritableSpace(needed + 1); // +1是为了'\0'

    va_start(args, fmt);
    int written = vsnprintf(beginWrite(), needed + 1, fmt, args);  // 返回值不包括'\0'
    va_end(args);
    if (written != needed) {
        LOGE("Buffer::appendFormatted error, written != needed");
        return -1;
    }
    mWriteIndex += written; // 网络传输不需要'\0'
    return written;
}

int Buffer::write(int clientFd)
{
    return sockets::write(clientFd, beginRead(), readableSize());
}
