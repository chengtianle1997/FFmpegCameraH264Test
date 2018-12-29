
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#include "iostream"

//#include "omp.h"

#include "MvCameraControl.h"

#include "afxwin.h"

#include "string.h"

//#include "Timer.h"





using namespace std;

#ifdef _WIN64



//Windows

extern "C"

{

#include "libavcodec/avcodec.h"

#include "libavformat/avformat.h"

#include "libavformat/avio.h"

#include "libavutil/opt.h"

#include "libavutil/imgutils.h"

};

#else

//Linux...

#ifdef __cplusplus

extern "C"

{

#endif

#include <libavcodec/avcodec.h>

#include <libavformat/avformat.h>

#ifdef __cplusplus

};

#endif

#endif

void* handle = NULL;//相机句柄

//stop_watch watch;



//打印相机设备信息
bool  PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
	if (pstMVDevInfo == NULL)
	{
		printf("The Pointer of pstMVDevInfo is NULL!\n");
		return false;
	}
	if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
	{
		printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
		printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
		printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
	}
	else
	{
		printf("Not Support!\n");
	}
	return true;
}
//
//int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
//	int ret;
//	int got_frame;
//	AVPacket enc_pkt;
//	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities & 0x0020))
//		return 0;
//	while (1) {
//		enc_pkt.data = NULL;
//		enc_pkt.size = 0;
//		av_init_packet(&enc_pkt);
//		ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
//			NULL, &got_frame);
//		av_frame_free(NULL);
//		if (ret < 0)
//			break;
//		if (!got_frame) {
//			ret = 0;
//			break;
//		}
//		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
//		/* mux encoded frame */
//		ret = av_write_frame(fmt_ctx, &enc_pkt);
//		if (ret < 0)
//			break;
//	}
//	return ret;
//}


int main(int argc, char* argv[])

{
	int nRet = MV_OK;
	unsigned int g_nPayloadSize = 0;
	//void* handle = NULL;
	//AVFormatContext* pFormatCtx;

	//AVOutputFormat* fmt;

	//AVStream* video_st;

	AVCodecContext* pCodecCtx;

	AVCodec* pCodec;

	//uint8_t* picture_buf;

	AVFrame* picture;

	AVPacket pkt;

	int y_size;

	int got_picture = 0;

	//int size;

	//int framecount = 100;

	int ret = 0;

	int i = 0;

	FILE *in_file = NULL;                            //YUV source

	FILE *fp_out = NULL;

	int in_w, in_h;       //YUV's width and height

	int CameraNum = 0;

	int thread = 1;

	//const char* out_file = "encode.h264";

	const char* out_file = "encode.mjpeg";    //Output file

	//unsigned char * pData = NULL;

	uint8_t *pDataForYUV = NULL;

	AVCodecID codec_id = AV_CODEC_ID_MJPEG;

	//in_file = fopen("ds_480x272.yuv", "rb");

	//获取设备枚举列表
	MV_CC_DEVICE_INFO_LIST stDevList;
	memset(&stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDevList);

	//const char* OrgPath = (char*)malloc(200);	
	//OrgPath = "D:\\Image";

	//bool flag = CreateDirectory(OrgPath, NULL);
	//if (!flag)
	//{
	//	printf("Create Folder Failed!\n");
	//}

	if (MV_OK != nRet)
	{
		printf("Enum Devices fail! nRet [0x%x]\n", nRet);
	}

	if (stDevList.nDeviceNum > 0)
	{
		for (unsigned int i = 0; i < stDevList.nDeviceNum; i++)
		{
			printf("[device %d]:\n", i);
			//设备信息
			MV_CC_DEVICE_INFO* pDeviceInfo = stDevList.pDeviceInfo[i];
			if (NULL == pDeviceInfo)
			{
				break;
			}
			PrintDeviceInfo(pDeviceInfo);
		}
	}
	else
	{
		printf("Find No Devices!\n");
	}


	//输入相机编号
	printf("Please Intput camera index:");
	unsigned int devNum = 0;
	scanf("%d", &devNum);
	//确认输入正确
	if (devNum >= stDevList.nDeviceNum)
	{
		printf("Intput error!\n");
	}
	//printf("Please Intput the frame count:");
	//scanf("%d", &framecount);

	//选择设备并创建句柄
	nRet = MV_CC_CreateHandle(&handle, stDevList.pDeviceInfo[devNum]);
	if (MV_OK != nRet)
	{
		printf("Create Handle fail! nRet [0x%x]\n", nRet);
	}
	nRet = MV_CC_OpenDevice(handle);
	if (MV_OK != nRet)
	{
		printf("Open Device fail! nRet [0x%x]\n", nRet);
	}
	else {
		printf("Device is ready!\n");
	}

	//设置触发模式为off
	nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
	if (MV_OK != nRet)
	{
		printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
	}

	//获取数据包大小
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
	if (MV_OK != nRet)
	{
		printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
	}
	g_nPayloadSize = stParam.nCurValue;

	//Get the width and Height of the Camera
	MVCC_INTVALUE pIntValue;
	memset(&pIntValue, 0, sizeof(MVCC_INTVALUE));
	MV_CC_GetIntValue(handle, "Width", &pIntValue);
	in_w = pIntValue.nCurValue;
	MV_CC_GetIntValue(handle, "Height", &pIntValue);
	in_h = pIntValue.nCurValue;
	//printf("%d*%d", in_w, in_h);



	//Set the properties of the camera
	//设置曝光时间
	float newExposureTime = 10000;
	nRet = MV_CC_SetFloatValue(handle, "ExposureTime", newExposureTime);
	if (MV_OK != nRet)
	{
		printf("Set ExposureTime fail! nRet [0x%x]\n", nRet);
	}
	//设置帧率
	float newAcquisitionFrameRate = 60.0;
	nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", newAcquisitionFrameRate);
	if (MV_OK != nRet)
	{
		printf("Set AcquisitionFrameRate fail! nRet [0x%x]\n", nRet);
	}
	//设置增益
	float newGain = 15.0;
	nRet = MV_CC_SetFloatValue(handle, "Gain", newGain);
	if (MV_OK != nRet)
	{
		printf("Set Gain fail! nRet [0x%x]\n", nRet);
	}

	printf("Please input the thread num:");
	scanf("%d", &thread);

	//开始取流
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));


	av_register_all();

	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		printf("Codec not found\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		printf("Could not allocate video codec context\n");
		return -1;
	}

	//pFormatCtx = avformat_alloc_context();
	//Guess Format
	//fmt = av_guess_format("h264", NULL, NULL);
	//pFormatCtx->oformat = fmt;

	//pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = AV_CODEC_ID_MJPEG;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->bit_rate = 4000000;
	pCodecCtx->width = 2592;
	pCodecCtx->height = 2048;
	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;
	pCodecCtx->gop_size = 10;
	//pCodecCtx->max_b_frames = 1;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ444P;
	pCodecCtx->thread_count = 2;
	pCodecCtx->framerate.num = 25;
	pCodecCtx->framerate.den = 1;
	//pCodecCtx->qmin = 10;
	//pCodecCtx->qmax = 51;

	pCodec->capabilities = AV_CODEC_CAP_SLICE_THREADS;

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		printf("Could not open codec\n");
		return -1;
	}

	picture = av_frame_alloc();
	if (!picture) {
		printf("Could not allocate video frame\n");
		return -1;
	}

	picture->width = pCodecCtx->width;

	picture->height = pCodecCtx->height;

	picture->format = pCodecCtx->pix_fmt;

	ret = av_image_alloc(picture->data, picture->linesize, pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 16);
	if (ret < 0) {
		printf("Could not allocate raw picture buffer\n");
		return -1;
	}



	//Show some Information
/*	av_dump_format(pFormatCtx, 0, out_file, 1);

	avcodec_parameters_from_context(video_st->codecpar, pCodecCtx);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);

	if (!pCodec) {

		printf("Codec not found.");

		return -1;

	}*/

	//Output bitstream
	fp_out = fopen(out_file, "wb");
	if (!fp_out) {
		printf("Could not open %s\n", out_file);
		return -1;
	}

	uint8_t startcode[] = { 0,0,0xff,0xd8 };

	fwrite(startcode, 1, sizeof(startcode), fp_out);

	y_size = pCodecCtx->width * pCodecCtx->height;
	
	int framecount = 0;

	//size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	int counter = 0;

	//Encode
	while (1) {

		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);

		

		if (nRet == MV_OK)
		{
			//watch.start();

			//YUV420 Planar
			pDataForYUV = (uint8_t*)malloc(stOutFrame.stFrameInfo.nWidth * stOutFrame.stFrameInfo.nHeight * 3);
			if (NULL == pDataForYUV)
			{
				printf("malloc pDataForYUV420 fail !\n");
				break;
			}



			//printf("%d",sizeof(stOutFrame.pBufAddr));

			//Read YUV

			//memcpy(picture_buf, stOutFrame.pBufAddr, stOutFrame.stFrameInfo.nHeight*stOutFrame.stFrameInfo.nWidth);

			//picture->data[0] = picture_buf;              // Y

			//picture->data[1] = picture_buf + y_size;      // U 

			//picture->data[2] = picture_buf + y_size * 5 / 4;  // V


			for (int j = 0; j < stOutFrame.stFrameInfo.nWidth*stOutFrame.stFrameInfo.nHeight; j++)
			{
				pDataForYUV[j] = (uint8_t)stOutFrame.pBufAddr[j];
				pDataForYUV[stOutFrame.stFrameInfo.nWidth*stOutFrame.stFrameInfo.nHeight + j] = 128;
				pDataForYUV[stOutFrame.stFrameInfo.nWidth*stOutFrame.stFrameInfo.nHeight * 2 + j] = 128;

			}

			av_init_packet(&pkt);
			pkt.data = NULL;    // packet data will be allocated by the encoder
			pkt.size = 0;
			//Read raw YUV data
			picture->data[0] = pDataForYUV;  //Y

			picture->data[1] = pDataForYUV + y_size;  //U

			picture->data[2] = pDataForYUV + y_size * 2;  //V

			picture->pts = i;

			//Encode

			ret = avcodec_encode_video2(pCodecCtx, &pkt, picture, &got_picture);

			if (ret < 0) {

				printf("Encode Error.\n");

				return -1;

			}

			if (got_picture) {
				printf("Succeed to encode frame: %5d\tsize:%5d\n", framecount, pkt.size);
				framecount++;
				fwrite(pkt.data, 1, pkt.size, fp_out);
				av_free_packet(&pkt);
			}

			/*if (got_picture ) {

				pkt.stream_index = video_st->index;

				ret = av_write_frame(pFormatCtx, &pkt);

			}*/

			i++;

			counter++;


			//watch.stop();

			/*if (watch.elapsed() > 1000000)
			{
				printf("\n");
				printf("The Framerate is %d fps\n", framecount);
				framecount = 0;
				watch.restart();
			}*/

			av_free_packet(&pkt);

			free(pDataForYUV);
		}
		else
		{
			printf("No data[0x%x]\n", nRet);
		}
		if (NULL != stOutFrame.pBufAddr)
		{
			nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
			if (nRet != MV_OK)
			{
				printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
			}
		}
		if (counter > 300)
			break;	
	}



	//avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//Write Header

	//avformat_write_header(pFormatCtx, NULL);

	//av_new_packet(&pkt, size);

	//y_size = pCodecCtx->width * pCodecCtx->height;

	//int framecount = 0;  //Counter



	////Flush Encoder
	//ret = flush_encoder(pFormatCtx, 0);
	//if (ret < 0) {
	//	printf("Flushing encoder failed\n");
	//	return -1;
	//}

	////Write file trailer
	//av_write_trailer(pFormatCtx);

	//printf("Encode Successful.\n");

	//Flush Encoder
	for (got_picture = 1; got_picture; i++) {
		ret = avcodec_encode_video2(pCodecCtx, &pkt, NULL, &got_picture);
		if (ret < 0) {
			printf("Error encoding frame\n");
			return -1;
		}
		if (got_picture) {
			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}

/*
	if (video_st) {

		avcodec_close(video_st->codec);

		av_free(picture);

		av_free(picture_buf);

	}*/

	/*avio_close(pFormatCtx->pb);

	avformat_free_context(pFormatCtx);
*/

	uint8_t endcode[] = { 0,0,0xff,0xd9 };

	//fwrite(endcode, 1, sizeof(endcode), fp_out);

	fclose(fp_out);
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	//av_freep(&picture->data[0]);
	av_frame_free(&picture);



	// ch:停止取流 | en:Stop grab image
	nRet = MV_CC_StopGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
	}

	// ch:关闭设备 | Close device
	nRet = MV_CC_CloseDevice(handle);
	if (MV_OK != nRet)
	{
		printf("ClosDevice fail! nRet [0x%x]\n", nRet);

	}

	// ch:销毁句柄 | Destroy handle
	nRet = MV_CC_DestroyHandle(handle);
	if (MV_OK != nRet)
	{
		printf("Destroy Handle fail! nRet [0x%x]\n", nRet);

	}


	if (nRet != MV_OK)
	{
		if (handle != NULL)
		{
			MV_CC_DestroyHandle(handle);
			handle = NULL;
		}
	}


	return 0;

}
