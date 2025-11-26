#pragma once
#include "MediaSource.h"
#include "luckfox_mpi.h"
#define DISP_WIDTH  1920
#define DISP_HEIGHT 1080

class LuckfoxCameraSource: public MediaSource
{
public:
    static LuckfoxCameraSource* createNew(UsageEnvironment* env, int width, int height);
    LuckfoxCameraSource(UsageEnvironment* env, int width, int height);
protected:
    virtual void handleTask();
private:
    int mWidth; // 摄像头图像宽度
    int mHeight; // 摄像图像高度
    char mFpsTxt[16]; // fps osd
    MB_POOL_CONFIG_S mPoolCfg; // 内存池配置文件
    MB_POOL mSrcPool; // 内存池
};
