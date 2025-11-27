#pragma once
#include "MediaSource.h"
#include "luckfox_mpi.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#define DISP_WIDTH  1920
#define DISP_HEIGHT 1080

class LuckfoxCameraSource: public MediaSource
{
public:
    static LuckfoxCameraSource* createNew(UsageEnvironment* env, int width = DISP_WIDTH, int height = DISP_HEIGHT);
    LuckfoxCameraSource(UsageEnvironment* env, int width, int height);
    ~LuckfoxCameraSource();
protected:
    virtual void handleTask();
private:
    int mWidth; // 摄像头图像宽度
    int mHeight; // 摄像图像高度
    char mFpsTxt[16]; // fps osd
    VENC_STREAM_S mStFrame;
    MB_POOL_CONFIG_S mPoolCfg; // 内存池配置文件
    MB_POOL mSrcPool; // 内存池
    MB_BLK mSrc_Blk;
    VIDEO_FRAME_INFO_S mH264_frame; 
    unsigned char *mData;
    cv::Mat mCvFrame;
    RK_U64 mH264_PTS;
    RK_U32 mH264_TimeRef;
    VIDEO_FRAME_INFO_S mStViFrame;
};
