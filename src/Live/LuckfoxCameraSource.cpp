#include "LuckfoxCameraSource.h"
#include "Log.h"

// model size
int model_width = 640;
int model_height = 640;	
float scale;
int leftPadding;
int topPadding;

static cv::Mat letterbox(cv::Mat input)
{
	float scaleX = (float)model_width  / (float)DISP_WIDTH; 
	float scaleY = (float)model_height / (float)DISP_HEIGHT; 
	scale = scaleX < scaleY ? scaleX : scaleY;
	
	int inputWidth   = (int)((float)DISP_WIDTH * scale);
	int inputHeight  = (int)((float)DISP_HEIGHT * scale);

	leftPadding = (model_width  - inputWidth) / 2;
	topPadding  = (model_height - inputHeight) / 2;	
	

	cv::Mat inputScale;
    cv::resize(input, inputScale, cv::Size(inputWidth,inputHeight), 0, 0, cv::INTER_LINEAR);	
	cv::Mat letterboxImage(640, 640, CV_8UC3,cv::Scalar(0, 0, 0));
    cv::Rect roi(leftPadding, topPadding, inputWidth, inputHeight);
    inputScale.copyTo(letterboxImage(roi));

	return letterboxImage; 	
}

static void mapCoordinates(int *x, int *y) {	
	int mx = *x - leftPadding;
	int my = *y - topPadding;

    *x = (int)((float)mx / scale);
    *y = (int)((float)my / scale);
}

// 计算两个 rect 的 IoU（float）
static float rect_iou(const cv::Rect_<float>& a, const cv::Rect_<float>& b) {
    float x1 = std::max(a.x, b.x);
    float y1 = std::max(a.y, b.y);
    float x2 = std::min(a.x + a.width, b.x + b.width);
    float y2 = std::min(a.y + a.height, b.y + b.height);

    float w = x2 - x1;
    float h = y2 - y1;
    if (w <= 0.0f || h <= 0.0f) return 0.0f;
    float inter = w * h;
    float areaA = a.width * a.height;
    float areaB = b.width * b.height;
    float uni = areaA + areaB - inter;
    if (uni <= 0.0f) return 0.0f;
    return inter / uni;
}

LuckfoxCameraSource* LuckfoxCameraSource::createNew(UsageEnvironment* env, int width, int height)
{
	return new LuckfoxCameraSource(env, width, height);
}

LuckfoxCameraSource::LuckfoxCameraSource(UsageEnvironment* env, int width, int height): MediaSource(env), 
                                        mWidth(width), mHeight(height)
{
	LOGI("LuckfoxCameraSource()");
	// TODO: 把模型初始化放在前面是因为这个构造函数后面会执行崩溃, 无法执行完成. 这可能是造成推流延时 模糊的原因
	const char *model_path = "./model/yolov5.rknn";
    memset(&mRknn_app_ctx, 0, sizeof(rknn_app_context_t));	
	int ret = init_yolov5_model(model_path, &mRknn_app_ctx);
	if (ret == 0)
		LOGI("init rknn model success!\n");
	else {
		LOGI("init rknn model failed! ret = %d\n", ret);
	}
	// delete model_path; // TODO: 为什么删掉这个指针会奔溃? ! a: 
	init_post_process();

	mSourceName = "LuckfoxCameraSource";
	system("RkLunch-stop.sh");
	setFps(11);
	memset(mFpsTxt, 0, 16);

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
	LOGI("RK_MPI_MB_GetMB()");

	// Build mH264_frame
	mH264_frame.stVFrame.u32Width = width;
	mH264_frame.stVFrame.u32Height = height;
	mH264_frame.stVFrame.u32VirWidth = width;
	mH264_frame.stVFrame.u32VirHeight = height;
	mH264_frame.stVFrame.enPixelFormat =  RK_FMT_RGB888; 
	mH264_frame.stVFrame.u32FrameFlag = 160;
	mH264_frame.stVFrame.pMbBlk = mSrc_Blk;
    mData = (unsigned char *)RK_MPI_MB_Handle2VirAddr(mSrc_Blk);
	LOGI("RK_MPI_MB_Handle2VirAddr");

	mCvFrame =  cv::Mat(cv::Size(width, height), CV_8UC3, mData);
	LOGI("cv::Mat()");

	// rkaiq init
	RK_BOOL multi_sensor = RK_FALSE;	
	const char *iq_dir = "/etc/iqfiles";
	rk_aiq_working_mode_t hdr_mode = RK_AIQ_WORKING_MODE_NORMAL;
	//hdr_mode = RK_AIQ_WORKING_MODE_ISP_HDR2;
	LOGI("before SAMPLE_COMM_ISP_Init");
	SAMPLE_COMM_ISP_Init(0, hdr_mode, multi_sensor, iq_dir);
	LOGI("SAMPLE_COMM_ISP_Init");
	SAMPLE_COMM_ISP_Run(0);
	LOGI("SAMPLE_COMM_ISP_Run");

	// rkmpi init
	if (RK_MPI_SYS_Init() != RK_SUCCESS) {
		RK_LOGE("rk mpi sys init fail!");
		return;
	}

	// vi init
	vi_dev_init();
	vi_chn_init(0, width, height);
	LOGI("vi_chn_init");

	// venc init
	RK_CODEC_ID_E enCodecType = RK_VIDEO_ID_AVC;
	venc_init(0, width, height, enCodecType);


    for(int i = 0; i < DEFAULT_FRAME_NUM; ++i)
    {
        mEnv->threadPool()->addTask(mTask);
    }

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

static inline bool startCode3(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] ==0 && buf[2] == 1)
        return true;
    else
        return false;
}

static inline bool startCode4(uint8_t* buf)
{
    if (buf[0] == 0 && buf[1] ==0 && buf[2] == 0 && buf[3] == 1)
        return true;
    else
        return false;
}

static uint8_t* findNextStartCode(uint8_t* buf, int size)
{
    if (size < 3)
        return nullptr;
    uint8_t* nextStartCode = buf;
    while (nextStartCode && (nextStartCode - buf) < size - 3)
    {
        if(startCode3(nextStartCode) || startCode4(nextStartCode))
            return nextStartCode;
        nextStartCode += 1;
    }
    if (startCode3(nextStartCode)) {
        return nextStartCode;
    }
    return nullptr;
}

void LuckfoxCameraSource::handleTask()
{
	std::lock_guard<std::mutex> lck(mMtx);
    if (mInputFrameQ.empty()) {
        LOGI("no input frame available");
        return;
    }
	LOGI("n_input : %d", mRknn_app_ctx.io_num.n_input);
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
			// LOGI("RK_MPI_VI_GetChnFrame success");
			void *vi_data = RK_MPI_MB_Handle2VirAddr(mStViFrame.stVFrame.pMbBlk);
			LOGI("read one frame");
			cv::Mat yuv420sp(mHeight + mHeight / 2, mWidth, CV_8UC1, vi_data);
			cv::Mat bgr(mHeight, mWidth, CV_8UC3, mData);			
			cv::cvtColor(yuv420sp, bgr, cv::COLOR_YUV420sp2BGR);
			cv::resize(bgr, mCvFrame, cv::Size(mWidth ,mHeight), 0, 0, cv::INTER_LINEAR);
			LOGI("resized one frame");
			//letterbox
			cv::Mat letterboxImage = letterbox(mCvFrame);	
			LOGI("letterbox one frame");
			memcpy(mRknn_app_ctx.input_mems[0]->virt_addr, letterboxImage.data, model_width*model_height*3);
			LOGI("memcpy one frame to virt_addr");
			inference_yolov5_model(&mRknn_app_ctx, &m_od_results);
			LOGI("inference one frame");
			for(int i = 0; i < m_od_results.count; i++)
			{					
				if(m_od_results.count >= 1)
				{
					object_detect_result *det_result = &(m_od_results.results[i]);
	
					int sX = (int)(det_result->box.left   );	
					int sY = (int)(det_result->box.top 	  );	
					int eX = (int)(det_result->box.right  );	
					int eY = (int)(det_result->box.bottom );
					mapCoordinates(&sX,&sY);
					mapCoordinates(&eX,&eY);
					
					printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(det_result->cls_id),
							 sX, sY, eX, eY, det_result->prop);

					cv::rectangle(mCvFrame,cv::Point(sX ,sY),
								        cv::Point(eX ,eY),
										cv::Scalar(0,255,0),3);
					sprintf(mFpsTxt, "%s %.1f%%", coco_cls_to_name(det_result->cls_id), det_result->prop * 100);
					cv::putText(mCvFrame, mFpsTxt,cv::Point(sX, sY - 8),
										   cv::FONT_HERSHEY_SIMPLEX,1,
										   cv::Scalar(0,255,0),2);
				}
			}
			
			// sprintf(mFpsTxt,"fps = %.2f", mFps);		
            // cv::putText(mCvFrame, mFpsTxt,
			// 				cv::Point(40, 40),
			// 				cv::FONT_HERSHEY_SIMPLEX,1,
			// 				cv::Scalar(0,255,0),2);

// **************************************************************************************************************************************
			// ---------- begin: SORT tracking logic (插入到 inference 后) ----------
			std::vector<cv::Rect_<float>> det_boxes;   // detections bbox in x,y,w,h (float)
			std::vector<float> det_scores;             // detection scores
			std::vector<int> det_cls_ids;              // detection class ids

			// 1) 收集目标类别为 m_target_class_id 的检测 (用 YOLO 输出)
			for (int i = 0; i < m_od_results.count; ++i) {
				object_detect_result *det_result = &(m_od_results.results[i]);

				// 如果只跟踪某个类别，跳过其他类别
				if (det_result->cls_id != m_target_class_id) continue;

				int sX = (int)(det_result->box.left);
				int sY = (int)(det_result->box.top);
				int eX = (int)(det_result->box.right);
				int eY = (int)(det_result->box.bottom);
				// mapCoordinates 如果改变了坐标，需要在此之后调用（你原来在框绘制前用了 mapCoordinates）
				mapCoordinates(&sX,&sY);
				mapCoordinates(&eX,&eY);

				// 转换为 x,y,w,h (float)
				float x = static_cast<float>(sX);
				float y = static_cast<float>(sY);
				float w = static_cast<float>(eX - sX);
				float h = static_cast<float>(eY - sY);
				if (w <= 0 || h <= 0) continue;

				det_boxes.emplace_back(x, y, w, h);
				det_scores.push_back(det_result->prop);
				det_cls_ids.push_back(det_result->cls_id);
			}

			// 2) 如果没有检测, 还是需要对已有 trackers 做 predict（以推进 time_since_update）
			int N = (int)det_boxes.size();
			int M = (int)m_trackers.size();

			// 先对每个 tracker 做 predict，并保留预测结果
			std::vector<cv::Rect_<float>> predicted_boxes;
			predicted_boxes.reserve(M);
			for (int k = 0; k < M; ++k) {
				cv::Rect_<float> pred = m_trackers[k].predict(); // KalmanTracker::predict() 返回 StateType (cv::Rect_<float>)
				predicted_boxes.push_back(pred);
			}

			// 3) 构造代价矩阵（N x M），cost = 1 - IoU  (lower cost == better match)
			std::vector<std::vector<double>> costMatrix;
			costMatrix.assign(N, std::vector<double>(std::max(1, M), 0.0)); // 注意当 M==0 时分配 1 列避免空

			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < M; ++j) {
					float iou = rect_iou(det_boxes[i], predicted_boxes[j]);
					costMatrix[i][j] = 1.0 - static_cast<double>(iou);
				}
				// if no trackers (M==0) costMatrix row stays length 1 with value 0
			}

			// 4) 调用 Hungarian 求解指派（如果 N>0 且 M>0）
			std::vector<int> assignment; // assignment.size == N, assignment[i] = assigned tracker index or -1
			assignment.assign(N, -1);

			if (N > 0 && M > 0) {
				double cost = m_hungarian.Solve(costMatrix, assignment); // assignment 填充
				// Hungarian 的实现通常会在无法匹配处填 -1
			}

			// 5) 根据 assignment 筛选 matches / unmatched
			std::vector<int> matchedDetIdx; matchedDetIdx.reserve(N);
			std::vector<int> matchedTrkIdx; matchedTrkIdx.reserve(M);
			std::vector<int> unmatchedDetIdx;
			std::vector<int> unmatchedTrkIdx;

			// mark trackers as unmatched initially
			std::vector<char> trkMatched(M, 0);

			// Process assignment, 并做 IOU threshold 过滤（对应 Python SORT 的行为）
			for (int i = 0; i < N; ++i) {
				int trkIdx = assignment[i];
				if (trkIdx >= 0 && trkIdx < M) {
					float iou = rect_iou(det_boxes[i], predicted_boxes[trkIdx]);
					if (iou >= m_iou_threshold) {
						// accept match
						matchedDetIdx.push_back(i);
						matchedTrkIdx.push_back(trkIdx);
						trkMatched[trkIdx] = 1;
					} else {
						// treat as unmatched (IOU 太低)
						assignment[i] = -1;
						unmatchedDetIdx.push_back(i);
					}
				} else {
					unmatchedDetIdx.push_back(i);
				}
			}
			// 未被匹配到的 tracker
			for (int j = 0; j < M; ++j) {
				if (!trkMatched[j]) unmatchedTrkIdx.push_back(j);
			}

			// 6) 更新 matched trackers，用对应 detection 调用 update()
			for (size_t k = 0; k < matchedDetIdx.size(); ++k) {
				int d = matchedDetIdx[k];
				int t = matchedTrkIdx[k];
				cv::Rect_<float> detRect = det_boxes[d];
				m_trackers[t].update(detRect); // KalmanTracker::update(stateMat)
			}

			// 7) 为 unmatched detections 创建新 tracker
			for (int idx : unmatchedDetIdx) {
				cv::Rect_<float> detRect = det_boxes[idx];
				KalmanTracker newTrk(detRect);
				m_trackers.push_back(newTrk);
			}

			// 8) 删除超过 max_age 的 tracker（从后向前删除以保持索引正确）
			for (int trk_i = (int)m_trackers.size() - 1; trk_i >= 0; --trk_i) {
				if (m_trackers[trk_i].m_time_since_update > m_max_age) {
					m_trackers.erase(m_trackers.begin() + trk_i);
				}
			}

			// 9) 输出/绘制当前“confirmed” tracks（类似 Python SORT 的输出规则）
			for (size_t k = 0; k < m_trackers.size(); ++k) {
				KalmanTracker &trk = m_trackers[k];
				// 只输出在当前帧被更新的，且 hit_streak >= min_hits，或帧数较早时宽松输出
				if ((trk.m_time_since_update < 1) && (trk.m_hit_streak >= m_min_hits || /*frame_count<=min_hits*/ true)) {
					cv::Rect_<float> box = trk.get_state(); // get_state() 返回 x,y,w,h
					// 将 float 转 int 以绘制
					cv::Rect rectToDraw(cv::Point((int)box.x, (int)box.y),
										cv::Size((int)box.width, (int)box.height));
					cv::rectangle(mCvFrame, rectToDraw, cv::Scalar(0, 0, 255), 2);

					// 绘制 ID (KalmanTracker 中 m_id 为 0-based 或基于实现)
					char idtxt[32];
					sprintf(idtxt, "ID:%d", trk.m_id); // 如果需要 +1 可自己调整
					cv::putText(mCvFrame, idtxt, cv::Point((int)box.x, (int)box.y - 6),
								cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2);
				}
			}
			// ---------- end: SORT tracking logic ----------
// **************************************************************************************************************************************
			
		} else {
			LOGI("get vi frame error %x", s32Ret);
		}
		memcpy(mData, mCvFrame.data, mWidth * mHeight * 3);
		// LOGI("memcpy from mCvFrame.data to mData");
		// encode H264	
		RK_MPI_VENC_SendFrame(0,  &mH264_frame ,-1);
		// LOGI("RK_MPI_VENC_SendFrame()");
		
		s32Ret = RK_MPI_VENC_GetStream(0, &mStFrame, -1);
		// LOGI("RK_MPI_VENC_GetStream()");
		if(s32Ret == RK_SUCCESS) {
			LOGI("len = %d PTS = %d \n",mStFrame.pstPack->u32Len, mStFrame.pstPack->u64PTS);	
			void *pData = RK_MPI_MB_Handle2VirAddr(mStFrame.pstPack->pMbBlk);
			RK_U64 nowUs = TEST_COMM_GetNowUs();
			// mFps = (float) 1000000 / (float)(nowUs - mH264_frame.stVFrame.u64PTS);			
			// memcpy(frame->temp, pData + mStFrame.pstPack->u32Offset, mStFrame.pstPack->u32Len); // 这里不能加这个偏移!!! 否则会找不到startCode
			memcpy(frame->temp, pData, mStFrame.pstPack->u32Len);
			frame->mBuf = frame->temp;
			frame->mSize = mStFrame.pstPack->u32Len;
			if (startCode3(frame->mBuf)) {
				frame->mBuf += 3;
				frame->mSize -= 3;
			}
			else if (startCode4(frame->mBuf)) {
				frame->mBuf += 4;
				frame->mSize -= 4;
			} else {
				LOGE("not found start code!");
			}
			// frame->mSize = 1920 * 1080 * 3;
			// LOGI("frame->mSize=%d, u64PTS=%d", frame->mSize, mStFrame.pstPack->u64PTS);
			// usleep(mStFrame.pstPack->u64PTS);
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
		// 睡眠50ms
		break;
	}
	mInputFrameQ.pop();
    mOutputFrameQ.push(frame);
}
