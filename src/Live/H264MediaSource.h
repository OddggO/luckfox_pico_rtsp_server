#pragma once
#include "MediaSource.h"
#include <string>

class H264MediaSource: public MediaSource
{
public:
    static H264MediaSource* createNew(UsageEnvironment* env, std::string source);
    H264MediaSource(UsageEnvironment* env, std::string source);
    virtual ~H264MediaSource();
protected:
    virtual void handleTask();
private:
    int getFrameFromH264File(uint8_t* frame, int size, int& startCode);
private:
    FILE* mFile;
};
