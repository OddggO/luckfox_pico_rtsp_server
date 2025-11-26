#include "LuckfoxCameraSource.h"
#include "Log.h"

LuckfoxCameraSource::LuckfoxCameraSource(UsageEnvironment* env, int width, int height): MediaSource(env), 
                                        mWidth(width), mHeight(height)
{
    // 设置内存池
    memset(&mPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    mPoolCfg.u64MBSize = mWidth * mHeight * 3;
    mPoolCfg.u32MBCnt = 1;
	mPoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
    mSrcPool = RK_MPI_MB_CreatePool(&mPoolCfg);
    LOGI("Create Pool success !");

    // Get MB from Pool 
	MB_BLK src_Blk = RK_MPI_MB_GetMB(mSrcPool, width * height * 3, RK_TRUE);

    	// Build h264_frame
	VIDEO_FRAME_INFO_S h264_frame;
	h264_frame.stVFrame.u32Width = width;
	h264_frame.stVFrame.u32Height = height;
	h264_frame.stVFrame.u32VirWidth = width;
	h264_frame.stVFrame.u32VirHeight = height;
	h264_frame.stVFrame.enPixelFormat =  RK_FMT_RGB888; 
	h264_frame.stVFrame.u32FrameFlag = 160;
	h264_frame.stVFrame.pMbBlk = src_Blk;
    unsigned char *data = (unsigned char *)RK_MPI_MB_Handle2VirAddr(src_Blk);
}
