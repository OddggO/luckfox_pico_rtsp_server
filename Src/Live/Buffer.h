#pragma once
#include <algorithm>
#include <assert.h>

class Buffer
{
private:
    // mBuffer 分为三块: 已读缓冲区 --- 读缓冲区 --- 写缓冲区
    char* mBuffer;
    int mBufferSize;
    int mReadIndex;
    int mWriteIndex;
public:
    static const int initialBufferSize;
    static const char* kCRLF;
    Buffer(int bufferSize = initialBufferSize);
    ~Buffer();

    char* beginBuffer() {return mBuffer;}
    const char* beginBuffer() const {return mBuffer;}
    char* beginRead() {return mBuffer + mReadIndex;} // 读缓冲区起始地址
    const char* beginRead() const {return mBuffer + mReadIndex;}
    char* beginWrite() {return mBuffer + mWriteIndex;}
    const char* beginWrite() const {return mBuffer + mWriteIndex;}

    int readableSize() const {return mWriteIndex - mReadIndex;}
    int writeableSize() const {return mBufferSize - mWriteIndex;}
    int prependableSize() const {return mReadIndex;} // 已读缓冲区大小

    const char* findNextCRLF() const
    {
        const char* crlf = std::search(beginRead(), beginWrite(), kCRLF, kCRLF + 2);
        // return crlf == beginWrite() ? NULL : crlf;
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findNextCRLF(const char* start) const
    {
        assert(start >= beginRead() && start < beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    
    const char* findLastCRLF() const
    {
        const char* crlf = std::find_end(beginRead(), beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }

    const char* findLastCRLF(const char* start) const
    {
        assert(start >= beginRead() && start < beginWrite());
        const char* crlf = std::find_end(start, beginWrite(), kCRLF, kCRLF + 2);
        return crlf == beginWrite() ? nullptr : crlf;
    }
    // 移动读指针，表示部分数据已读
    void retrievalAll() {mReadIndex = 0; mWriteIndex = 0;} // 恢复索引
    // 已读len个字节，读指针向前移动
    void retrieval(int len) 
    {
        assert(len <= readableSize());
        if (len < readableSize()) {
            mReadIndex += len;
        } else { // len == readableSize()，读完所有数据，恢复索引
            retrievalAll();
        }
    }
    // 已读到end位置，读指针向后移动
    void retrievalUntil(const char* end) 
    {
        assert(end >= beginRead() && end <= beginWrite());
        retrieval(end - beginRead());
    }

    // 确保有足够空间
    void ensureWritableSpace(int len);
    void makeSpace(int len);

    int append(const char* data, int len); // 将data的len个字节内容写入缓冲区（复制到可写缓冲区）
    int append(const void* data, int len);

    int read(int clientFd); // 从网络io读取内容到写缓冲区
    // int read(const char* str, int size); // 将str的内容读取到缓冲区
    // 网络格式化写入，不包含终止符'\0'。如果要将读缓冲区内容作为字符串使用，需要手动添加'\0'，
    // 或者用std::string构造函数，输入起始指针和结束指针。
    int appendFormatted(const char* fmt, ...); // 按照格式字符串写入
    int write(int clientFd);

};
