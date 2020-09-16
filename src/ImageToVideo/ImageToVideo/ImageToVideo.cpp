#include "ImageToVideo.h"
#include <stdint.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

#pragma comment(lib, "vfw32")

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ImageToVideo");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
#endif

int32_t main()
{
	std::string currentPath = CSystem::GetCurrentExePath();
#ifdef _WIN64
	if (!CSystem::DirOrFileExist(currentPath + "opencv_ffmpeg320_64.dll"))
	{
		printf("cannot find opencv_ffmpeg320_64.dll\n");
		getchar();
		return -1;
	}
#elif _WIN32
	if (!CSystem::DirOrFileExist(currentPath + "opencv_ffmpeg320.dll"))
	{
		printf("cannot find opencv_ffmpeg320.dll\n");
		getchar();
		return -1;
	}
#endif

	std::vector<std::string> vecParam = CSystem::exeParam();
	if (vecParam.size() != 2 && vecParam.size() != 6)
	{
		printf("param error\n");
		return -1;
	}

	//保存的视频文件名
	std::string flieName;
	//图片路径格式
	std::string pathFormat;
	//图片开始帧号
	int startFrame = 0;
	//图片结束帧号
	int endFrame = 0;
	//帧率
	double fps = 0;

#ifdef _WIN32
	std::string level = "\\";
#elif __linux__
	std::string level = "/";
#endif

	if (vecParam.size() == 2)
	{
		std::string dir = vecParam[1].c_str();
		flieName = currentPath + CSystem::GetName(dir, 1) + ".avi";
		pathFormat = dir + level + "%d.jpg";
		startFrame = 1;
		endFrame = atoi(CSystem::inputString("请输入结束帧序号").c_str());
		fps = atof(CSystem::inputString("请输入帧率").c_str());
	}
	else
	{
		flieName = vecParam[1].c_str();
		pathFormat = vecParam[2].c_str();
		startFrame = atoi(vecParam[3].c_str());
		endFrame = atoi(vecParam[4].c_str());
		fps = atof(vecParam[5].c_str());
	}

	std::string path;
	IplImage* pImg = nullptr;
	//图片宽度
	int imgWidth = 0;
	//图片高度
	int imgHeight = 0;
	while (startFrame <= endFrame)
	{
		path = CStringManager::Format(pathFormat.c_str(), startFrame);
		pImg = cvLoadImage(path.c_str());
		if (pImg != nullptr)
		{
			//图片宽度
			imgWidth = pImg->width;
			//图片高度
			imgHeight = pImg->height;
			break;
		}
		printf("warning cannot open file, path = %s\n", path.c_str());
		++startFrame;
	}

	if (pImg == nullptr)
	{
		printf("cannot find path format = %s\n", pathFormat.c_str());
		return -1;
	}

	//OpenCV 库函数，创建视频写入器
	CvVideoWriter* pWriter = cvCreateVideoWriter(flieName.c_str(), CV_FOURCC('X', 'V', 'I', 'D'), fps, cvSize(imgWidth, imgHeight));
	if (pWriter == nullptr)
	{
		printf("create CvVideoWriter error, fileName = %s, fps = %d, imgWidth = %d, imgHeight = %d\n", flieName.c_str(), fps, imgWidth, imgHeight);
		return -1;
	}

	cvWriteFrame(pWriter, pImg);
	RCSend("write frame %s", path.c_str());
	cvReleaseImage(&pImg);

	while (++startFrame <= endFrame)
	{
		//这里的设置适合形如 123.jpg 124.jpg的图片序列
		path = CStringManager::Format(pathFormat.c_str(), startFrame);
		pImg = cvLoadImage(path.c_str());
		if (pImg == nullptr)
		{
			printf("warning cannot open file, path = %s\n", path.c_str());
			continue;
		}
		cvWriteFrame(pWriter, pImg);
		RCSend("write frame %s", path.c_str());
		cvReleaseImage(&pImg);
	}

	//释放资源
	cvReleaseVideoWriter(&pWriter);
	return 0;
}