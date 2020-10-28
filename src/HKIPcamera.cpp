//HKIPcamera.cpp
#include <opencv2/opencv.hpp>
#include <iostream>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <list>
#include "HCNetSDK.h"
#include "LinuxPlayM4.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#define USECOLOR 1
#define WINAPI

using namespace cv;
using namespace std;

//--------------------------------------------
int iPicNum = 0;//Set channel NO.
LONG nPort = -1;
HWND hWnd = NULL;
pthread_mutex_t g_cs_frameList;
list<char *> g_frameList;
int image_width;
int image_height;
LONG lUserID;
NET_DVR_DEVICEINFO_V30 struDeviceInfo;
LONG lRealPlayHandle = -1;

//解码回调 视频为YUV数据(YV12)，音频为PCM数据
void CALLBACK DecCBFun(int nPort, char * pBuf, int nSize, FRAME_INFO * pFrameInfo, void * nReserved1, int nReserved2)
{
	// struct timeval tv;
	// gettimeofday(&tv,NULL);
	// printf("millisecond:%ld\n", tv.tv_sec*1000 + tv.tv_usec/1000);  //毫秒

	long lFrameType = pFrameInfo->nType;

	if (lFrameType == T_YV12)
	{
		pthread_mutex_lock(&g_cs_frameList);
		g_frameList.push_back(pBuf);
		image_width = pFrameInfo->nWidth;
		image_height = pFrameInfo->nHeight;
		pthread_mutex_unlock(&g_cs_frameList);
	}
}

///实时流回调
void CALLBACK fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
	DWORD dRet;
	BOOL inData = FALSE;

	switch (dwDataType)
	{
		case NET_DVR_SYSHEAD:    //系统头
			if (!PlayM4_GetPort(&nPort)) //获取播放库未使用的通道号
				break;

			if (dwBufSize > 0)
			{
				if (!PlayM4_SetStreamOpenMode(nPort, STREAME_REALTIME))  //设置实时流播放模式
					break;

				if (!PlayM4_OpenStream(nPort, pBuffer, dwBufSize, 6 * 1024 * 1024))
					break;

				//设置解码回调函数 只解码不显示
				if (!PlayM4_SetDecCallBackMend(nPort, DecCBFun, NULL))
					break;

				//打开视频解码
				if (!PlayM4_Play(nPort, hWnd))
					break;
			}
			break;


		case NET_DVR_STREAMDATA:   //码流数据
			if (dwBufSize > 0 && nPort != -1)
			{
				if (!PlayM4_InputData(nPort, pBuffer, dwBufSize))
				{
					cout << ("PlayM4_InputData failed") << endl;
					break;
				}
			}
			break;

		default:	
			if (dwBufSize > 0 && nPort != -1)
			{
				if (!PlayM4_InputData(nPort, pBuffer, dwBufSize))
				{
					cout << ("PlayM4_InputData failed") << endl;
					break;
				}
			}
			break;
	}
}

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = { 0 };
	switch (dwType)
	{
		case EXCEPTION_RECONNECT:    //预览时重连
			printf("----------reconnect--------%d\n", time(NULL));
			break;
		default:
			break;
	}
}

void *ReadCamera(void *IpParameter)
{
	//---------------------------------------
	//设置异常消息回调函数
	NET_DVR_SetExceptionCallBack_V30(0, NULL, g_ExceptionCallBack, NULL);

	//---------------------------------------
	//启动预览并设置回调数据流
	NET_DVR_PREVIEWINFO StruPlayInfo = { 0 };
	StruPlayInfo.hPlayWnd = NULL;  //窗口为空，设备SDK不解码只取流  
	StruPlayInfo.lChannel = 1;     //预览通道号
	StruPlayInfo.dwStreamType = 0; //0-主流码，1-子流码，2-流码3，3-流码4，以此类推
	StruPlayInfo.dwLinkMode = 0;   //0-TCP方式，1-UDP方式，2-多播方式，3-RTP方式，4-RTP/RTSP，5-RSTP/HTTP
	StruPlayInfo.bBlocked = 1;     //0-非堵塞取流，1-堵塞取流

	LONG lRealPlayHandle;
	lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &StruPlayInfo, fRealDataCallBack, NULL);

	if (lRealPlayHandle<0)
		printf("NET_DVR_RealPlay_V30 failed! Error number: %d\n", NET_DVR_GetLastError());
	else
		cout << "The program is successful!!" << endl;

	sleep(-1);
}

bool OpenCamera(char *ip, char* usr, char* password)
{
	lUserID = NET_DVR_Login_V30(ip, 8000, usr, password, &struDeviceInfo);
	if (lUserID == 0)
	{
		return TRUE;
	}
	else
	{
		NET_DVR_Cleanup();
		return FALSE;
	}
}

void init(char* ip, char* usr, char* password)
{
	pthread_t hThread;
	NET_DVR_Init();
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(10000, true);
	OpenCamera(ip, usr, password);
	pthread_mutex_init(&g_cs_frameList, NULL);
	pthread_create(&hThread, NULL, ReadCamera, NULL);
}

void yv12toYUV(char *outYuv, char *inYv12, int width, int height, int widthStep)
{
	int col, row;
	unsigned int Y, U, V;
	int tmp;
	int idx;

	for (row = 0; row<height; row++)
	{
		idx = row * widthStep;
		int rowptr = row*width;


		for (col = 0; col<width; col++)
		{
			tmp = (row / 2)*(width / 2) + (col / 2);
			Y = (unsigned int)inYv12[row*width + col];
			U = (unsigned int)inYv12[width*height + width*height / 4 + tmp];
			V = (unsigned int)inYv12[width*height + tmp];

			outYuv[idx + col * 3] = Y;
			outYuv[idx + col * 3 + 1] = U;
			outYuv[idx + col * 3 + 2] = V;
		}
	}
}

Mat getframe()
{
	char * dbgframe;
 	struct timeval t1,t2;
 	double timeuse;

	pthread_mutex_lock(&g_cs_frameList);

	while (!g_frameList.size())
	{	
		pthread_mutex_unlock(&g_cs_frameList);
		pthread_mutex_lock(&g_cs_frameList);
	}

	list<char * >::iterator it;
	it = g_frameList.end();
	it--;
	dbgframe = (*(it));

	// gettimeofday(&t1,NULL);

	static IplImage* pImgYCrCb = cvCreateImage(cvSize(image_width, image_height), 8, 3);//得到图像的Y分量  
	yv12toYUV(pImgYCrCb->imageData, dbgframe, image_width, image_height, pImgYCrCb->widthStep);//得到所有RGB图像
	static IplImage* pImg = cvCreateImage(cvSize(image_width, image_height), 8, 3);
	cvCvtColor(pImgYCrCb, pImg, CV_YCrCb2RGB);

	// gettimeofday(&t2,NULL);
	// timeuse = (t2.tv_sec - t1.tv_sec) + (double)(t2.tv_usec - t1.tv_usec)/1000000.0;

	g_frameList.clear();

	pthread_mutex_unlock(&g_cs_frameList);
	
	Mat image_rgb=cvarrToMat(pImg);
	return(image_rgb);
}

void release()
{
	NET_DVR_StopRealPlay(lRealPlayHandle);

	//释放播放库资源
	PlayM4_Stop(nPort);
	PlayM4_CloseStream(nPort);
	PlayM4_FreePort(nPort);

	//注销用户
	NET_DVR_Logout(lUserID);
	NET_DVR_Cleanup();
}




