#include "LuckfoxCameraSource.h"
#include "Log.h"

LuckfoxCameraSource* LuckfoxCameraSource::createNew(UsageEnvironment* env, int width, int height)
{
	return new LuckfoxCameraSource(env, width, height);
}

LuckfoxCameraSource::LuckfoxCameraSource(UsageEnvironment* env, int width, int height): MediaSource(env), 
                                        mWidth(width), mHeight(height)
{
	LOGI("LuckfoxCameraSource()");
	mSourceName = "LuckfoxCameraSource";
	setFps(30);
	//h264_frame	
	mStFrame.pstPack = (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S));
	mH264_PTS = 0;
	mH264_TimeRef = 0; 

    // 设置内存池
    memset(&mPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    mPoolCfg.u64MBSize = mWidth * mHeight * 3;
    mPoolCfg.u32MBCnt = 1;
	mPoolCfg.enAllocType = MB_ALLOC_TYPE_DMA;
    mSrcPool = RK_MPI_MB_CreatePool(&mPoolCfg);
    LOGI("Create Pool success !");

    // Get MB from Pool 
	mSrc_Blk = RK_MPI_MB_GetMB(mSrcPool, width * height * 3, RK_TRUE);

	// Build mH264_frame
	mH264_frame.stVFrame.u32Width = width;
	mH264_frame.stVFrame.u32Height = height;
	mH264_frame.stVFrame.u32VirWidth = width;
	mH264_frame.stVFrame.u32VirHeight = height;
	mH264_frame.stVFrame.enPixelFormat =  RK_FMT_RGB888; 
	mH264_frame.stVFrame.u32FrameFlag = 160;
	mH264_frame.stVFrame.pMbBlk = mSrc_Blk;
    mData = (unsigned char *)RK_MPI_MB_Handle2VirAddr(mSrc_Blk);

	mCvFrame =  cv::Mat(cv::Size(width, height), CV_8UC3, mData);

	// rkaiq init
	RK_BOOL multi_sensor = RK_FALSE;	
	const char *iq_dir = "/etc/iqfiles";
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	//hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
	SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
	SAMPLE_COMM_ISP_Run(0);

	// rkmpi init
	if (RK_MPI_SYS_Init() != RK_SUCCESS) {
		RK_LOGE("rk mpi sys init fail!");
		return;
	}

	// vi init
	vi_dev_init();
	vi_chn_init(0, width, height);

	// venc init
	RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
	venc_init(0, width, height, enCodecType);

	LOGI("init success");
}

LuckfoxCameraSource::~LuckfoxCameraSource()
{
	// Destory MB
	RK_MPI_MB_ReleaseMB(mSrc_Blk);
	// Destory Pool
	RK_MPI_MB_DestroyPool(mSrcPool);

	RK_MPI_VI_DisableChn(0, 0);
	RK_MPI_VI_DisableDev(0);
		
	SAMPLE_COMM_ISP_Stop(0);

	RK_MPI_VENC_StopRecvFrame(0);
	RK_MPI_VENC_DestroyChn(0);

	free(mStFrame.pstPack);

	// RK_MPI_SYS_Exit(); // TODO: 这里不能调用，否则会导致其他MediaSource无法工作
}

void LuckfoxCameraSource::handleTask()
{
	std::lock_guard<std::mutex> lck(mMtx);
    if (mInputFrameQ.empty()) {
        LOGI("no input frame available");
        return;
    }
	MediaFrame* frame = mInputFrameQ.front();
	RK_S32 s32Ret = 0;
	while (true)
	{
		// get vi frame
		mH264_frame.stVFrame.u32TimeRef = mH264_TimeRef++;
		mH264_frame.stVFrame.u64PTS = TEST_COMM_GetNowUs(); 
		s32Ret = RK_MPI_VI_GetChnFrame(0, 0, &mStViFrame, -1);
		if(s32Ret == RK_SUCCESS)
		{
			void *vi_data = RK_MPI_MB_Handle2VirAddr(mStViFrame.stVFrame.pMbBlk);

			cv::Mat yuv420sp(mHeight + mHeight / 2, mWidth, CV_8UC1, vi_data);
			cv::Mat bgr(mHeight, mWidth, CV_8UC3, mData);			
			cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
			cv::resize(bgr, mCvFrame, cv::Size(mWidth ,mHeight), 0, 0, cv::INTER_LINEAR);
			
			sprintf(mFpsTxt,"fps = %.2f", mFps);		
            cv::putText(mCvFrame, mFpsTxt,
							cv::Point(40, 40),
							cv::FONT_HERSHEY_SIMPLEX,1,
							cv::Scalar(0,255,0),2);
			
		} else {
			LOGI("get vi frame error %x", s32Ret);
		}
		memcpy(mData, mCvFrame.data, mWidth * mHeight * 3);

		// encode H264	
		RK_MPI_VENC_SendFrame(0,  &mH264_frame ,-1);
		
		s32Ret = RK_MPI_VENC_GetStream(0, &mStFrame, -1);
		if(s32Ret == RK_SUCCESS) {
			LOGI("len = %d PTS = %d \n",mStFrame.pstPack->u32Len, mStFrame.pstPack->u64PTS);	
			void *pData = RK_MPI_MB_Handle2VirAddr(mStFrame.pstPack->pMbBlk);
			RK_U64 nowUs = TEST_COMM_GetNowUs();
			// mFps = (float) 1000000 / (float)(nowUs - mH264_frame.stVFrame.u64PTS);			
			memcpy(frame->temp, pData + mStFrame.pstPack->u32Offset, mStFrame.pstPack->u32Len);
			frame->mBuf = frame->temp;
			frame->mSize = mStFrame.pstPack->u32Len;
		} else {
			LOGI("get stream error %x", s32Ret);
		}
		// release vi frame
		s32Ret = RK_MPI_VI_ReleaseChnFrame(0, 0, &mStViFrame);
		if (s32Ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_VI_ReleaseChnFrame fail %x", s32Ret);
		}
		s32Ret = RK_MPI_VENC_ReleaseStream(0, &mStFrame);
		if (s32Ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_VENC_ReleaseStream fail %x", s32Ret);
		}
		break;
	}
	mInputFrameQ.pop();
    mOutputFrameQ.push(frame);
}
