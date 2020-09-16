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
		//�ر��˳��¼�
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

	//�������Ƶ�ļ���
	std::string flieName;
	//ͼƬ·����ʽ
	std::string pathFormat;
	//ͼƬ��ʼ֡��
	int startFrame = 0;
	//ͼƬ����֡��
	int endFrame = 0;
	//֡��
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
		endFrame = atoi(CSystem::inputString("���������֡���").c_str());
		fps = atof(CSystem::inputString("������֡��").c_str());
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
	//ͼƬ���
	int imgWidth = 0;
	//ͼƬ�߶�
	int imgHeight = 0;
	while (startFrame <= endFrame)
	{
		path = CStringManager::Format(pathFormat.c_str(), startFrame);
		pImg = cvLoadImage(path.c_str());
		if (pImg != nullptr)
		{
			//ͼƬ���
			imgWidth = pImg->width;
			//ͼƬ�߶�
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

	//OpenCV �⺯����������Ƶд����
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
		//����������ʺ����� 123.jpg 124.jpg��ͼƬ����
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

	//�ͷ���Դ
	cvReleaseVideoWriter(&pWriter);
	return 0;
}