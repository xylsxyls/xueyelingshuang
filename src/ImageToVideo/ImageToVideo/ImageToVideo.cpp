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
		//�ر��˳��¼�
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
	//�������Ƶ�ļ���
	const char* flieName = vecParam[1].c_str();
	//ͼƬ·����ʽ
	const char* pathFormat = vecParam[2].c_str();
	//ͼƬ��ʼ֡��
	int startFrame = atoi(vecParam[3].c_str());
	//ͼƬ����֡��
	int endFrame = atoi(vecParam[4].c_str());
	//֡��
	double fps = atoi(vecParam[5].c_str());

	std::string path = CStringManager::Format(pathFormat, startFrame);
	IplImage* pImg = cvLoadImage(path.c_str());
	if (pImg == nullptr)
	{
		printf("cannot open file, path = %s\n", path.c_str());
		return -1;
	}
	//ͼƬ���
	int imgWidth = pImg->width;
	//ͼƬ�߶�
	int imgHeight = pImg->height;

	//OpenCV �⺯����������Ƶд����
	CvVideoWriter* pWriter = cvCreateVideoWriter(flieName, CV_FOURCC('X', 'V', 'I', 'D'), fps, cvSize(imgWidth, imgHeight));

	cvWriteFrame(pWriter, pImg);
	RCSend("write frame %s", path.c_str());
	cvReleaseImage(&pImg);

	while (++startFrame <= endFrame)
	{
		//����������ʺ����� 123.jpg 124.jpg��ͼƬ����
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

	//�ͷ���Դ
	cvReleaseVideoWriter(&pWriter);
	return 0;
}