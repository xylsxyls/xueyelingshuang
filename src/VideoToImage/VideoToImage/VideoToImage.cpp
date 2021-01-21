#include "VideoToImage.h"
#include <stdint.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

#pragma comment(lib, "vfw32")

class SaveTask : public CTask
{
public:
	SaveTask():
		m_img(nullptr)
	{

	}

public:
	void DoTask()
	{
		cvSaveImage(m_path.c_str(), m_img, nullptr);
		cvReleaseImage(&m_img);
		m_img = nullptr;
	}

	void setParam(const std::string& path, IplImage* img)
	{
		m_path = path;
		m_img = img;
	}

protected:
	std::string m_path;
	IplImage* m_img;
};

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close VideoToImage");
		CTaskThreadManager::Instance().UninitAll();
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
	if (vecParam.size() != 2 && vecParam.size() != 4)
	{
		printf("param error\n");
		return -1;
	}

	//��Ƶ�ļ�·��
	std::string fileName;
	//�洢·��
	std::string pathFormat;
	//ͼƬ��ʼ���
	int frame = 0;

#ifdef _WIN32
	std::string level = "\\";
#elif __unix__
	std::string level = "/";
#endif

	if (vecParam.size() == 2)
	{
		fileName = vecParam[1].c_str();
		std::string videoName = CSystem::GetName(fileName, 3);
		CSystem::CreateDir(currentPath + videoName);
		pathFormat = currentPath + videoName + level + "%d.jpg";
	}
	else
	{
		fileName = vecParam[1].c_str();
		pathFormat = vecParam[2].c_str();
		frame = atoi(vecParam[3].c_str()) - 1;
	}

	CvCapture* cap = cvCaptureFromFile(fileName.c_str());
	if (cap == nullptr)
	{
		printf("cannot open video\n");
		return -1;
	}

	std::vector<uint32_t> vecThreadId;
	int32_t threadCount = CSystem::GetCPUCoreCount() * 2 + 1;
	int32_t index = -1;
	while (index++ != threadCount - 1)
	{
		vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}

	index = -1;
	//����ͼƬ���ļ���·�� һ��Ҫ�У���ΪOpenCV�����Զ������ļ���
	IplImage* pImg = nullptr;
	while ((pImg = cvQueryFrame(cap)) != nullptr)
	{
		//����������ʺ����� 123.jpg 124.jpg��ͼƬ����
		std::shared_ptr<SaveTask> spSaveTask(new SaveTask);
		spSaveTask->setParam(CStringManager::Format(pathFormat.c_str(), ++frame), cvCloneImage(pImg));
		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[(++index) % threadCount])->PostTask(spSaveTask);
	}
	cvReleaseCapture(&cap);

	index = -1;
	while (index++ != threadCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(vecThreadId[index]);
	}

	return 0;
}