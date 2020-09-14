#include "ImageToVideo.h"
#include <stdint.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

#pragma comment(lib, "opencv_world320.lib")

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
	std::vector<std::string> vecParam = CSystem::exeParam();
	if (vecParam.size() != 6)
	{
		printf("param error\n");
		return -1;
	}
	//保存的视频文件名
	const char* flieName = vecParam[1].c_str();
	//图片路径格式
	const char* pathFormat = vecParam[2].c_str();
	//图片开始帧号
	int startFrame = atoi(vecParam[3].c_str());
	//图片结束帧号
	int endFrame = atoi(vecParam[4].c_str());
	//帧率
	double fps = atoi(vecParam[5].c_str());

	std::string path = CStringManager::Format(pathFormat, startFrame);
	IplImage* pImg = cvLoadImage(path.c_str());
	if (pImg == nullptr)
	{
		printf("cannot open file, path = %s\n", path.c_str());
		return -1;
	}
	//图片宽度
	int imgWidth = pImg->width;
	//图片高度
	int imgHeight = pImg->height;

	//OpenCV 库函数，创建视频写入器
	CvVideoWriter* pWriter = cvCreateVideoWriter(flieName, CV_FOURCC('X', 'V', 'I', 'D'), fps, cvSize(imgWidth, imgHeight));

	cvWriteFrame(pWriter, pImg);
	RCSend("write frame %s", path.c_str());
	cvReleaseImage(&pImg);

	while (++startFrame <= endFrame)
	{
		//这里的设置适合形如 123.jpg 124.jpg的图片序列
		path = CStringManager::Format(pathFormat, startFrame);
		pImg = cvLoadImage(path.c_str());
		if (pImg == nullptr)
		{
			printf("cannot open file, path = %s\n", path.c_str());
			cvReleaseVideoWriter(&pWriter);
			return -1;
		}
		cvWriteFrame(pWriter, pImg);
		RCSend("write frame %s", path.c_str());
		cvReleaseImage(&pImg);
	}

	//释放资源
	cvReleaseVideoWriter(&pWriter);
	return 0;
}