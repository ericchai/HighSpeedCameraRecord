#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

//#include <time>
using std::filesystem::directory_iterator;
using std::filesystem::path;
//using std::filesystem::v1::directory_iterator;
//using std::filesystem::v1::path;
//using std::experimental::filesystem::v1::directory_iterator;
//using std::experimental::filesystem::v1::path;

#include <opencv.hpp>
#include <atomic>
#include "windows.h"

#define IM_width 640
#define IM_highth 480
#define FPS 790
#define IM_size IM_width*IM_highth

struct SharedMemory
{
	UINT index;                 // 相机回调索引
	UINT  TimeStampL;           // 相机帧时间戳低位
	UINT  TimeStampH;           // 相机帧时间戳高位
	unsigned char data[IM_size];

	int  hCamera;                //相机句柄
	int  capturetime;            //录像时间
	HANDLE hfile;
};

struct imdata
{
	imdata()
		:index{ 0 }
	{
		std::memset(data, 0, IM_size);
	}
	UINT index;             // 相机回调索引
	unsigned char data[IM_size];
};


class CreateVedio
{
public:
	CreateVedio();
	double fps;
	int w;
	int h;
	cv::Mat BGim;
	std::string datapath;                                   // .dat文件路径
	std::string vediopath;                                  // 视频文件路径
	std::string impath;	                                    // 图片文件路径
	std::string bgimpath;
	//std::vector<int64> imstamps;//从data中读取每一行的时间戳数据，然后
	//SharedMemory* pData;


	void genBGim();                                          // 生成背景
	bool checkBG(cv::Mat inputIm, int thresholdvalve);       // 检查背景
	void readData2imSingle();                                // 单线程生成图片
	void readData2imMuti();                                  // 多线程生成图片
	void readData2imMutiByFilter();                          // 多线程背景不同生成图片
	void im2vedio();                                         // opencv生成视频
	void im2vediobyff();                                     // 调用ffmpga程序生成视频
	

	void handleDatFile(std::string datname);
	void handleDatFileByFilter(std::string datname);
};

int deleteAllFile(const char* filedir);
int deleteAllFile(std::string&& filedir);
int deleteAllFile(std::string& filedir);
int deleteAllFile(std::string filedir);

DWORD WINAPI ReadFileContent(LPCTSTR lpPath, PBYTE& pData, DWORD& dwSize);
int orderBmpAndChangeName(const char* filedir);              // 背景过滤过的图片重新排序并且重新命名

