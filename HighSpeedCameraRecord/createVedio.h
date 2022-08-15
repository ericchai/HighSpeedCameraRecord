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
	UINT index;                 // ����ص�����
	UINT  TimeStampL;           // ���֡ʱ�����λ
	UINT  TimeStampH;           // ���֡ʱ�����λ
	unsigned char data[IM_size];

	int  hCamera;                //������
	int  capturetime;            //¼��ʱ��
	HANDLE hfile;
};

struct imdata
{
	imdata()
		:index{ 0 }
	{
		std::memset(data, 0, IM_size);
	}
	UINT index;             // ����ص�����
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
	std::string datapath;                                   // .dat�ļ�·��
	std::string vediopath;                                  // ��Ƶ�ļ�·��
	std::string impath;	                                    // ͼƬ�ļ�·��
	std::string bgimpath;
	//std::vector<int64> imstamps;//��data�ж�ȡÿһ�е�ʱ������ݣ�Ȼ��
	//SharedMemory* pData;


	void genBGim();                                          // ���ɱ���
	bool checkBG(cv::Mat inputIm, int thresholdvalve);       // ��鱳��
	void readData2imSingle();                                // ���߳�����ͼƬ
	void readData2imMuti();                                  // ���߳�����ͼƬ
	void readData2imMutiByFilter();                          // ���̱߳�����ͬ����ͼƬ
	void im2vedio();                                         // opencv������Ƶ
	void im2vediobyff();                                     // ����ffmpga����������Ƶ
	

	void handleDatFile(std::string datname);
	void handleDatFileByFilter(std::string datname);
};

int deleteAllFile(const char* filedir);
int deleteAllFile(std::string&& filedir);
int deleteAllFile(std::string& filedir);
int deleteAllFile(std::string filedir);

DWORD WINAPI ReadFileContent(LPCTSTR lpPath, PBYTE& pData, DWORD& dwSize);
int orderBmpAndChangeName(const char* filedir);              // �������˹���ͼƬ������������������

