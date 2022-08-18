// HighSpeedCameraRecord.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
//BIG5 TRANS ALLOWED
//#include "stdafx.h"
#include <stdio.h>
#include <tchar.h>
#include "windows.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <iostream>
#include <atomic>
#include <chrono>
#include "filemapmem.h"
#include "createVedio.h"
#include <CameraApi.h>

#include <format>

#include <process.h>
#include <SDKDDKVer.h>

#ifdef _WIN64
#pragma comment(lib, "..\\HighSpeedCameraRecord\\MVCAMSDK_X64.lib")
#else
#pragma comment(lib, "..\\HighSpeedCameraRecord\\MVCAMSDK.lib")
#endif

using namespace std;
using namespace cv;

// ---------------------------------------------------------------------------
#define WIDTH            IM_width
#define HEIGHT           IM_highth
#define SIZEIMAGE        IM_size
#define TOTALSIZEIMAGE   IM_size+4
#define MAPCOUNT         10
#define IMAGECOUNTINMAP  IM_countonemap
filemapmem* f1[MAPCOUNT];
bool creatflag[MAPCOUNT];

uint deleteMapcount{ 0 };

HANDLE hFile;
HANDLE hFileMapping;
LPVOID pMapHandle;
// -------------------------------------------------------------------

UINT            m_threadID;		        // 图像抓取线程的ID
HANDLE          m_hDispThread;	        // 图像抓取线程的句柄
BOOL            m_bExit = FALSE;		// 用来通知图像抓取线程结束
CameraHandle    m_hCamera;		        // 相机句柄，多个相机同时使用时，可以用数组代替	
BYTE* m_pFrameBuffer;                   // 用于将原始图像数据转换为RGB的缓冲区
tSdkFrameHead   m_sFrInfo;		        // 用于保存当前图像帧的帧头信息

std::atomic_bool wirteflag{ false };

int	            m_iDispFrameNum{ 0 };	// 用于记录当前已经显示的图像帧的数量
uint32_t	    m_igetFrameNum{ 0 };	// 用于记录当前已经取得的图像帧的数量
float           m_fDispFps;			    // 显示帧率
float           m_fCapFps;			    // 捕获帧率
tSdkFrameStatistic  m_sFrameCount;
tSdkFrameStatistic  m_sFrameLast;
int					m_iTimeLast;
char		    g_CameraName[64];

/*
USE_CALLBACK_GRAB_IMAGE
如果需要使用回调函数的方式获得图像数据，则反注释宏定义USE_CALLBACK_GRAB_IMAGE.
我们的SDK同时支持回调函数和主动调用接口抓取图像的方式。两种方式都采用了"零拷贝"机制，以最大的程度的降低系统负荷，提高程序执行效率。
但是主动抓取方式比回调函数的方式更加灵活，可以设置超时等待时间等，我们建议您使用 uiDisplayThread 中的方式
*/
//#define USE_CALLBACK_GRAB_IMAGE 

#ifdef USE_CALLBACK_GRAB_IMAGE
/*图像抓取回调函数*/
IplImage* g_iplImage = NULL;

void _stdcall GrabImageCallback(CameraHandle hCamera, BYTE* pFrameBuffer, tSdkFrameHead* pFrameHead, PVOID pContext)
{

	CameraSdkStatus status;


	//将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
	//我公司大部分型号的相机，原始数据都是Bayer格式的
	status = CameraImageProcess(hCamera, pFrameBuffer, m_pFrameBuffer, pFrameHead);

	//分辨率改变了，则刷新背景
	if (m_sFrInfo.iWidth != pFrameHead->iWidth || m_sFrInfo.iHeight != pFrameHead->iHeight)
	{
		m_sFrInfo.iWidth = pFrameHead->iWidth;
		m_sFrInfo.iHeight = pFrameHead->iHeight;
	}

	if (status == CAMERA_STATUS_SUCCESS)
	{
		//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
		CameraImageOverlay(hCamera, m_pFrameBuffer, pFrameHead);
		if (g_iplImage)
		{
			cvReleaseImageHeader(&g_iplImage);
		}
		g_iplImage = cvCreateImageHeader(cvSize(pFrameHead->iWidth, pFrameHead->iHeight), IPL_DEPTH_8U, sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3);
		cvSetData(g_iplImage, m_pFrameBuffer, pFrameHead->iWidth * (sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3));
		cvShowImage(g_CameraName, g_iplImage);
		m_iDispFrameNum++;
		waitKey(30);
	}

	memcpy(&m_sFrInfo, pFrameHead, sizeof(tSdkFrameHead));

}

#else 
/*图像抓取线程，主动调用SDK接口函数获取图像*/
UINT WINAPI uiDisplayThread(LPVOID lpParam)
{
	tSdkFrameHead 	sFrameInfo;
	CameraHandle    hCamera = (CameraHandle)lpParam;
	BYTE* pbyBuffer;
	CameraSdkStatus status;
	/*IplImage *iplImage = NULL;*/

	//auto start_  = std::chrono::steady_clock::now();
	//auto stop_ = start_;

	while (!m_bExit)
	{
		// 读取图像
		/// \param [in] hCamera 相机的句柄。
		/// \param [out] pFrameInfo 图像的帧头信息指针。
		/// \param [out] pbyBuffer 返回图像数据的缓冲区指针。
		/// \param [in] wTimes 抓取图像的超时时间，单位毫秒。在wTimes时间内还未获得图像，则该函数会返回超时错误。
		/// \return 成功返回 CAMERA_STATUS_SUCCESS(0)。否则返回非0值的错误码, 请参考 CameraStatus.h 中错误码的定义。
		if (CameraGetImageBuffer(hCamera, &sFrameInfo, &pbyBuffer, 1000) == CAMERA_STATUS_SUCCESS)
		{
			// 将获得的原始数据转换成RGB格式的数据，同时经过ISP模块，对图像进行降噪，边沿提升，颜色校正等处理。
			// 我公司大部分型号的相机，原始数据都是Bayer格式的
			/// \param [in] hCamera 相机的句柄。
			/// \param [in] pbyIn 输入图像数据的缓冲区地址，不能为NULL。 
			/// \param [out] pbyOut 处理后图像输出的缓冲区地址，不能为NULL。
			/// \param [inout] pFrInfo 输入图像的帧头信息，处理完成后，帧头信息中的图像格式uiMediaType会随之改变。
			/// \return 成功返回 CAMERA_STATUS_SUCCESS(0)。否则返回非0值的错误码, 请参考 CameraStatus.h 中错误码的定义。
			//status = CameraImageProcess(hCamera, pbyBuffer, m_pFrameBuffer,&sFrameInfo); //连续模式
			status = CameraImageProcessEx(hCamera, pbyBuffer, m_pFrameBuffer, &sFrameInfo, CAMERA_MEDIA_TYPE_MONO8, 0);
			// pbyBuffer的byte长度Width*Height  m_pFrameBuffer的byte长度为Width*Height*3（RGB）


			////分辨率改变了，则刷新背景
			//if (m_sFrInfo.iWidth != sFrameInfo.iWidth || m_sFrInfo.iHeight != sFrameInfo.iHeight)
			//{
			//	m_sFrInfo.iWidth = sFrameInfo.iWidth;
			//	m_sFrInfo.iHeight = sFrameInfo.iHeight;
			//	//图像大小改变，通知重绘
			//}

			if (status == CAMERA_STATUS_SUCCESS)  // CAMERA_STATUS_SUCCESS   0   操作成功 
			{
				//调用SDK封装好的显示接口来显示图像,您也可以将m_pFrameBuffer中的RGB数据通过其他方式显示，比如directX,OpengGL,等方式。
				//CameraImageOverlay(hCamera, m_pFrameBuffer, &sFrameInfo);
				// 由于SDK输出的数据默认是从底到顶的，转换为Opencv图片需要做一下垂直镜像
				// CameraFlipFrameBuffer(m_pFrameBuffer, &sFrameInfo, 1);

#if 0
				if (iplImage)
				{
					cvReleaseImageHeader(&iplImage);
				}
				iplImage = cvCreateImageHeader(cvSize(sFrameInfo.iWidth, sFrameInfo.iHeight), IPL_DEPTH_8U, sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3);
				cvSetData(iplImage, m_pFrameBuffer, sFrameInfo.iWidth * (sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? 1 : 3));
				cvShowImage(g_CameraName, iplImage);
#else
				/*cv::Mat matImage(
					cv::Size(sFrameInfo.iWidth,sFrameInfo.iHeight),
					sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
					m_pFrameBuffer
					);
				imshow(g_CameraName, matImage);*/
#endif          
				if (wirteflag == true)
				{
					if (m_igetFrameNum < IMAGECOUNTINMAP * MAPCOUNT)
					{
						if (m_igetFrameNum == 1)
						{
							// 关闭背景显示窗口
							cv::destroyWindow(g_CameraName);
						}

						uint mapindex = m_igetFrameNum / IMAGECOUNTINMAP;
						uint shfit = m_igetFrameNum % IMAGECOUNTINMAP;
						uint64_t num = shfit * (TOTALSIZEIMAGE);

						char* pStr = (char*)f1[mapindex]->pMapHandle + num;
						std::memcpy(pStr, &m_igetFrameNum, 4);
						pStr += 4;
						std::memcpy(pStr, m_pFrameBuffer, SIZEIMAGE);
						//std::memcpy(pStr, pbyBuffer, SIZEIMAGE);

						//if (m_igetFrameNum % 775 == 10)
						//{
						//	start_ = stop_;
						//	stop_ = std::chrono::steady_clock::now();
						//	auto duration1 = (stop_ - start_);
						//	//auto myns = std::chrono::duration_cast<std::chrono::microseconds>(duration1).count();
						//	auto myms = std::chrono::duration_cast<std::chrono::milliseconds>(duration1).count();
						//	std::cout << m_igetFrameNum <<" <-> "<< myms << ":ms " << std::endl;
						//}
						if (m_igetFrameNum % 1000 == 10)
						{
							std::cout << "shot number is->" << m_igetFrameNum << std::endl;
						}

						m_igetFrameNum++;

						if (m_igetFrameNum >= IMAGECOUNTINMAP * MAPCOUNT)
						{
							m_bExit = true;
						}
					}
				}
				else
				{
					cv::Mat matImage(
						cv::Size(sFrameInfo.iWidth, sFrameInfo.iHeight),
						sFrameInfo.uiMediaType == CAMERA_MEDIA_TYPE_MONO8 ? CV_8UC1 : CV_8UC3,
						m_pFrameBuffer
					);
					cv::imshow(g_CameraName, matImage);
					waitKey(10);
				}
				//m_igetFrameNum++;
				//m_iDispFrameNum++;
			}
			//在成功调用CameraGetImageBuffer后，必须调用CameraReleaseImageBuffer来释放获得的buffer。
			//否则再次调用CameraGetImageBuffer时，程序将被挂起，知道其他线程中调用CameraReleaseImageBuffer来释放了buffer
			CameraReleaseImageBuffer(hCamera, pbyBuffer);
			std::memcpy(&m_sFrInfo, &sFrameInfo, sizeof(tSdkFrameHead));
		}

		/*std::this_thread::sleep_for(std::chrono::milliseconds(10));
		if (capCount >= 10000)
		{
			m_bExit = TRUE;
			break;
		}*/

		//int c = waitKey(10);   // 10ms

		//if (c == 'q' || c == 'Q' || (c & 255) == 27)
		//{
		//	m_bExit = TRUE;
		//	break;
		//}
		if (m_bExit)
		{
			break;
		}

	}

	//if (iplImage)
	//{
	//	cvReleaseImageHeader(&iplImage);
	//}

	_endthreadex(0);
	return 0;
}
#endif


// 线程函数，主要作用是开辟和释放文件映射内存
void funcfilemapmem()
{
	while (true)
	{
		uint index = m_igetFrameNum / IMAGECOUNTINMAP;
		if (index < 1)
		{
		}
		else if (index >= 1 && index <= (MAPCOUNT - 3))
		{
			if (creatflag[index] == false)
			{
				delete f1[index - 1];
				deleteMapcount++;
				auto str = std::string("c:\\usb30data\\") + std::to_string(index + 2) + std::string(".dat");
				f1[index + 2] = new filemapmem(str.c_str(), std::to_string(index + 2).c_str());
				creatflag[index] = true;
			}
		}
		else if (index <= MAPCOUNT)
		{
			if (creatflag[index] == false)
			{
				delete f1[index - 1];
				deleteMapcount++;
				creatflag[index] = true;
			}
		}
		else
		{
		}
		std::this_thread::sleep_for(std::chrono::microseconds(5));
	}

}

// main
int _tmain(int argc, _TCHAR* argv[])
{
#if 1
	tSdkCameraDevInfo sCameraList[10];    // 相机的设备信息
	INT iCameraNums;                      // 相机的数目
	CameraSdkStatus status;               // SDK错误码
	tSdkCameraCapbility sCameraInfo;      // 设备描述信息

	SYSTEM_INFO sinf;
	GetSystemInfo(&sinf);
	DWORD dwAllocationGranularity = sinf.dwAllocationGranularity;     // 655635 <-> 0xffff <-> 64k
	std::cout << "program start........" << std::endl;

	deleteAllFile("c:\\usb30data");
	deleteAllFile("d:\\DATAOUTPUT\\image");

	for (int i = 0; i < MAPCOUNT; i++)
	{
		creatflag[i] = false;
	}

	// 先开3块文件映射
	for (int i = 0; i < 3; i++)
	{
		auto str = std::string("c:\\usb30data\\") + std::to_string(i) + std::string(".dat");
		f1[i] = new filemapmem(str.c_str(), std::to_string(i).c_str());
	}

	// test
	// 此处是一个循环,一直在往文件里写数据.
	/*char* pStr = (char*)pMapHandle;
	strcpy(pStr, "221221221");*/

	//char s[100];
	//strcpy(s, pStr);


	//枚举设备，获得设备列表
	//调用CameraEnumerateDevice前，先设置iCameraNums = 10，表示最多只读取10个设备，如果需要枚举更多的设备，请更改sCameraList数组的大小和iCameraNums的值
	iCameraNums = 10;

	if (CameraEnumerateDevice(sCameraList, &iCameraNums) != CAMERA_STATUS_SUCCESS || iCameraNums == 0)
	{
		printf("No camera was found!");
		return FALSE;
	}

	//该示例中，我们只假设连接了一个相机。因此，只初始化第一个相机。(-1,-1)表示加载上次退出前保存的参数，如果是第一次使用该相机，则加载默认参数.
	//In this demo ,we just init the first camera.
	if ((status = CameraInit(&sCameraList[0], -1, -1, &m_hCamera)) != CAMERA_STATUS_SUCCESS)
	{
		char msg[128];
		sprintf_s(msg, "Failed to init the camera! Error code is %d", status);
		printf(msg);
		printf(CameraGetErrorString(status));
		return FALSE;
	}

	//Get properties description for this camera.
	//"获得该相机的特性描述"
	CameraGetCapability(m_hCamera, &sCameraInfo);

	// 申请一段对齐的内存空间。功能和malloc类似，但是返回的内存是以align指定的字节数对齐的。
	/// \param [in] size	 空间的大小。 
	/// \param [in] align    地址对齐的字节数。
	/// \return 成功时，返回非0值，表示内存首地址。失败返回NULL。
	m_pFrameBuffer = (BYTE*)CameraAlignMalloc(sCameraInfo.sResolutionRange.iWidthMax * sCameraInfo.sResolutionRange.iWidthMax * 3, 16);

	// 黑白CMOS
	if (sCameraInfo.sIspCapacity.bMonoSensor)
	{
		CameraSetIspOutFormat(m_hCamera, CAMERA_MEDIA_TYPE_MONO8);
	}

	// 得到相机名字
	strcpy_s(g_CameraName, sCameraList[0].acFriendlyName);

	// 创建该相机的属性配置窗口。
	//CameraCreateSettingPage(m_hCamera,NULL,
	//	g_CameraName,NULL,NULL,0);//"通知SDK内部建该相机的属性页面";

#ifdef USE_CALLBACK_GRAB_IMAGE //如果要使用回调函数方式，定义USE_CALLBACK_GRAB_IMAGE这个宏
	//Set the callback for image capture
	CameraSetCallbackFunction(m_hCamera, GrabImageCallback, 0, NULL);//"设置图像抓取的回调函数";
#else
	// 启动相机数据采集线程
	m_hDispThread = (HANDLE)_beginthreadex(NULL, 0, &uiDisplayThread, (PVOID)m_hCamera, 0, &m_threadID);
#endif

	// 启动动态开辟和关闭文件映射内存
	std::thread(funcfilemapmem).detach();

	// 让相机进入工作模式，开始接收来自相机发送的图像数据。
	CameraPlay(m_hCamera);
	// CameraShowSettingPage(m_hCamera,TRUE);//TRUE显示相机配置界面。FALSE则隐藏。
	std::cout << "相机开始初始化........." << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(10));
	std::cout << "相机初始化完成！\n";
	std::cout << "shot number is->" << m_igetFrameNum << std::endl;
	// 设置有效采集开始
	wirteflag = true;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	// 关闭有效采集前显示的背景图片窗口
	// cv::destroyWindow(g_CameraName);

	// printf("note:input q is quit.\n");

	while (m_bExit != true)
	{
		int c = waitKey(10);
		if (c == 'q')
		{
			m_bExit = true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
	// 相机反初始化。释放资源。
	CameraUnInit(m_hCamera);
	// 释放由@link #CameraAlignMalloc @endlink函数分配的内存空间。
	CameraAlignFree(m_pFrameBuffer);
	// cv::destroyWindow(g_CameraName);

	//while (true)
	//{
	//	if (deleteMapcount >= 20)
	//	{
	//		break;
	//	}
	//}

#endif

#if 1
	// 处理文件
	CreateVedio cvv;
	//cvv.readData2im();
	cvv.readData2imMuti();
	//cvv.readData2imMutiByFilter();
	// 图片处理线程是join，阻塞模式
	//std::cout << "bmp图片生成完成！" << std::endl;
	//orderBmpAndChangeName("d:\\DATAOUTPUT\\image");
	//cvv.im2vediobyff();
	//std::cout << "视频生成完成！" << std::endl;

	std::cout << "输入d删除原始文件和图片文件退出，或者等待10s不删除自动退出" << std::endl;

	while (true)
	{
		// 等待10s没有输入则退出
		int c = waitKey(1000);
		if (c == 'd')
		{
			//deleteAllFile("c:\\usb30data");
			//deleteAllFile("d:\\DATAOUTPUT\\image");
		}
		break;
	}

#endif
	//UnmapViewOfFile(pMapHandle);
	//CloseHandle(hFileMapping);
	//CloseHandle(hFile);
#ifdef USE_CALLBACK_GRAB_IMAGE
	if (g_iplImage)
	{
		cvReleaseImageHeader(&g_iplImage);
	}
#endif
	return 0;
}
