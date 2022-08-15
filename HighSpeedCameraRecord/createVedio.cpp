#include "createVedio.h"


// 背景阈值
#define THRESHOLDVALVE 100

// 构造
CreateVedio::CreateVedio()
	:fps(60.0)
	,w(IM_width)
	,h(IM_highth)
	,datapath("c:\\usb30data")
	,vediopath("D:\\DATAOUTPUT\\vedio")
	,impath("D:\\DATAOUTPUT\\image")
{
	BGim = cv::Mat::zeros(cv::Size(w, h), CV_8UC1);
	genBGim();
}


// 线程函数，不带背景过滤
void CreateVedio::handleDatFile(std::string datname)
{
	PBYTE pData;
	DWORD dwSize;
	imdata* onedata = new imdata();
	cv::Mat im(IM_highth, IM_width, CV_8UC1);
	ReadFileContent(datname.c_str(), pData, dwSize);
	//int imnum = dwSize / sizeof(imdata);
	int imnum = IM_countonemap;
	for (int i = 0; i < imnum; ++i)
	{
		std::memcpy(onedata, pData, sizeof(imdata));
		im.data = onedata->data;
		std::string fmt = impath + "\\%d.bmp";
		char tempstrpath[256];
		int temstrlenpath = snprintf(tempstrpath, sizeof(tempstrpath), fmt.c_str(), onedata->index);
		std::string imageName(tempstrpath);
		cv::imwrite(imageName, im);
		pData += sizeof(imdata);
	}
	delete onedata;

	std::cout << datname << " done!\n";
}

// 线程函数,带背景过滤
void CreateVedio::handleDatFileByFilter(std::string datname)
{
	PBYTE pData;
	DWORD dwSize;
	imdata* onedata = new imdata();
	cv::Mat im(IM_highth, IM_width, CV_8UC1);
	ReadFileContent(datname.c_str(), pData, dwSize);
	//int imnum = dwSize / sizeof(imdata);
	int imnum = IM_countonemap;
	for (int i = 0; i < imnum; ++i)
	{
		std::memcpy(onedata, pData, sizeof(imdata));
		im.data = onedata->data;
		//im.data = pData;		

		if (checkBG(im, THRESHOLDVALVE))
		{
			std::string fmt = impath + "\\%d.bmp";
			char tempstrpath[256];
			int temstrlenpath = snprintf(tempstrpath, sizeof(tempstrpath), fmt.c_str(), onedata->index);
			std::string imageName(tempstrpath);
			cv::imwrite(imageName, im);
		}

		pData += sizeof(imdata);
	}
	delete onedata;
	std::cout << datname << " done!\n";
}

// 多线程生成图片
void CreateVedio::readData2imMuti()
{
	for (auto& v : directory_iterator(datapath))
	{
		std::string fileName = v.path().filename().string();
		std::string extensionName = v.path().extension().string();
		//如果后缀是“dat”，则是需要读取的数据
		if (extensionName == ".dat")
		{
			std::string str = v.path().string();
			std::thread(std::bind(&CreateVedio::handleDatFile,this,str)).join();
		}	
	}
}

// 多线程生成图片带背景过滤
void CreateVedio::readData2imMutiByFilter()
{
	for (auto& v : directory_iterator(datapath))
	{
		std::string fileName = v.path().filename().string();
		std::string extensionName = v.path().extension().string();
		//如果后缀是“dat”，则是需要读取的数据
		if (extensionName == ".dat")
		{
			std::string str = v.path().string();
			std::thread(std::bind(&CreateVedio::handleDatFileByFilter,this,str)).join();
		}
	}
}

// 单线程生成图片
void CreateVedio::readData2imSingle()
{
	int count = 0;
	imdata* onedata = new imdata();
	cv::Mat im(IM_highth, IM_width, CV_8UC1);
	for (auto& v : directory_iterator(datapath))
	{
		std::string fileName = v.path().filename().string();
		std::string extensionName = v.path().extension().string();
		//如果后缀是“dat”，则是需要读取的数据
		if (extensionName == ".dat")
		{
			PBYTE pData;
			DWORD dwSize;
			ReadFileContent(v.path().string().c_str(), pData, dwSize);
			//int imnum = dwSize / sizeof(imdata);
			int imnum = IM_countonemap;
			for (int i = 0; i < imnum; ++i)
			{
				std::memcpy(onedata, pData, sizeof(imdata));
				im.data = onedata->data;

				std::string fmt = impath + "\\%d.bmp";
				char tempstrpath[256];
				int temstrlenpath = snprintf(tempstrpath, sizeof(tempstrpath), fmt.c_str(), onedata->index);
				std::string imageName(tempstrpath);
				cv::imwrite(imageName, im);
				pData += sizeof(imdata);
			}
		}
	}
	delete onedata;
}

// 生成背景
void CreateVedio::genBGim()
{
	int count = 0;
	imdata* onedata = new imdata();
	cv::Mat im(IM_highth, IM_width, CV_8UC1, cv::Scalar(0));
	cv::Mat imF(IM_highth, IM_width, CV_32FC1, cv::Scalar(0));
	cv::Mat bg_F;
	std::string path0 = datapath + "\\0.dat";
	PBYTE pData;
	DWORD dwSize;

	ReadFileContent(path0.c_str(), pData, dwSize);
	for (int i = 100; i < 200; ++i)  //只计算100-110之间10张的平均
	{
		std::memcpy(onedata, pData, sizeof(imdata));
		im.data = onedata->data;
		im.convertTo(bg_F, CV_32FC1);
		imF = imF + bg_F;
		pData += sizeof(imdata);
	}
	imF = imF / 100;
	imF.convertTo(BGim, CV_8UC3);

	delete onedata;
}

// 检查背景
bool CreateVedio::checkBG(cv::Mat inputIm, int thresholdvalve)
{
	cv::Mat diffgray;
	cv::absdiff(BGim, inputIm, diffgray);
	// 背景灯是否正常开启，取灰度值255的20% 即51。超过存在超过51，认为背景灯没有开
	cv::Mat mask;
	cv::threshold(diffgray, mask, thresholdvalve, 255, cv::THRESH_BINARY);
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(mask, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

	int num = contours.size();
	if (num > 0)  //有对象
	{
		return true;
	}

	return false;
}

// 根据图片生成视频
void CreateVedio::im2vedio()
{
	//获得图像名列表
	std::vector<std::string> imnames;
	for (auto& v : directory_iterator(impath))
	{
		std::string fileName = v.path().string();
		std::string extensionName = v.path().extension().string();
		//如果后缀是“dat”，则是需要读取的数据
		PBYTE pData;
		DWORD dwSize;
		if (extensionName == ".bmp")
		{
			imnames.push_back(v.path().string());
		}
	}
	std::sort(imnames.begin(), imnames.end());


	time_t timenow;
	time(&timenow);
	struct tm localt;
	localtime_s(&localt, &timenow);

	char savename[64] = { 0 };
	snprintf(savename, sizeof(savename) - 1, "\\snap-%d_%d_%d_%d_%d_%d.avi",
		localt.tm_year + 1900,
		localt.tm_mon + 1,
		localt.tm_mday,
		localt.tm_hour,
		localt.tm_min,
		localt.tm_sec);
	std::string name(savename);

	cv::VideoWriter writer;
	std::string video_name = vediopath + name;
	//writer =cv::VideoWriter(video_name, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(w, h), false);
	writer = cv::VideoWriter(video_name, cv::CAP_OPENCV_MJPEG, fps, cv::Size(w, h), false);
	//writer.open(video_name, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, cv::Size(w, h), false); //open方法是先要调用release方法，再构造
	if (!writer.isOpened())
	{
		std::cout << "writer is not open!" << std::endl;
		return;
	}

	cv::Mat img;
	/*cv::Mat bgimg = cv::imread(bgimpath);;
	cv::Mat diffimg(h, w, CV_8UC1, Scalar(0));*/

	uint32_t nums = imnames.size();
	for (int i = 0; i < nums; ++i)
	{
		std::string imname = imnames[i];
		img = cv::imread(imname);
		writer.write(img);

		/*cv::absdiff(img, bgimg, diffimg);
		cv::Mat mask = diffimg>150;


		vector<vector<Point> > contours;
		cv::findContours(mask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);

		for (int i = 0; i < contours.size(); ++i)
		{
			double a = cv::contourArea(contours[i], false);
			if (a > 200)
			{
				writer.write(img);
				break;
			}
		}*/

		//	
		//	//cv::imshow("Live", img);
		//	//if (cv::waitKey(5) >= 0)
		//	//	break;
	}


}


// 根据图片生成视频使用ffmpeg
void CreateVedio::im2vediobyff()
{
	//获得图像名列表
	system("d:\\ffmpeg.exe -y -framerate 30 -start_number 0 -i d:\\DATAOUTPUT\\image\\%d.bmp -vcodec libx264 d:\\DATAOUTPUT\\vedio\\a.mp4");
}

// 删除一个文件夹下的所有文件
int deleteAllFile(const char* filedir)
{
	namespace fs = std::filesystem;
	int filecount = 0;
	auto temp = fs::path(filedir);
	if (fs::is_empty(temp))
	{
		std::cout << "image文件夹为空！" << std::endl;
		return 0;
	}
	for (auto& v : directory_iterator(filedir))
	{
		// 得到文件名
		std::string fileName = v.path().string();
		// 得到文件的扩展名
		// std::string extensionName = v.path().extension().string();
		std::remove(fileName.c_str());
		filecount++;
	}
	return filecount;
	//
	//fs::path tmp = fs::path("d:\\DATAOUTPUT\\image2");
	//std::uintmax_t n = fs::remove_all(tmp);
	//std::cout << "deleted" << n << "file or dir\n";
}

// 删除一个文件夹下的所有文件
int deleteAllFile(std::string&& filedir)
{
	return deleteAllFile(filedir.c_str());
}

// 删除一个文件夹下的所有文件
int deleteAllFile(std::string& filedir)
{
	return deleteAllFile(filedir.c_str());
}

// 删除一个文件夹下的所有文件
int deleteAllFile(std::string filedir)
{
	return deleteAllFile(filedir.c_str());
}

// 背景过滤过的图片重新排序并且重新命名
int orderBmpAndChangeName(const char* filedir)
{
	namespace fs = std::filesystem;
	std::set<std::string> bmpset;

	int filecount = 0;
	auto temp = fs::path(filedir);
	if (fs::is_empty(temp))
	{
		std::cout << "该文件夹为空！" << std::endl;
		return 0;
	}
	for (auto& v : directory_iterator(filedir))
	{
		// 得到文件名
		std::string fileName = v.path().string();
		// 得到文件的扩展名
		std::string extensionName = v.path().extension().string();
		if (extensionName == ".bmp")
		{
			bmpset.emplace(fileName);
		}
		filecount++;
	}

	int i = 0;
	for (auto& str : bmpset)
	{
		auto temp = std::string(filedir) + std::string("\\") + std::to_string(i) + std::string(".bmp");
		auto dst = fs::path(temp);
		auto src = fs::path(str);
	    fs:rename(src, dst);
		i++;
	}

	return filecount;
}



// 读文件内容
DWORD WINAPI ReadFileContent(LPCTSTR lpPath, PBYTE& pData, DWORD& dwSize)
{
	DWORD  dwErr = NO_ERROR;
	HANDLE hFile = CreateFile(lpPath, FILE_GENERIC_READ,            //  打开文件，获得文件读句柄
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,     //  共享方式打开，避免其他地方需要读写此文件
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (INVALID_HANDLE_VALUE == hFile)                              //  文件打开失败，返回错误值
		return GetLastError();

	dwSize = GetFileSize(hFile, NULL);                              //  取文件大小，这里的文件不能太大，否则需要分段读取文件
	pData = (PBYTE)LocalAlloc(LPTR, dwSize);                       //  申请缓冲区，下面的 ReadFile 里面会判断这里申请是否成功

	if (FALSE == ReadFile(hFile, pData, dwSize, &dwSize, NULL))     //  读取文件失败，记录失败错误值
		dwErr = GetLastError();

	CloseHandle(hFile);                                             //  关闭文件句柄，避免句柄泄露
	return dwErr;                                                   //  返回错误值，NO_ERROR 代表没有任何错误
}



