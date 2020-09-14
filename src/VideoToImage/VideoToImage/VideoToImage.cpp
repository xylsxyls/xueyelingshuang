#include "VideoToImage.h"
#include <stdint.h>
#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

#pragma comment(lib, "opencv_world320.lib")

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
	std::vector<std::string> vecParam = CSystem::exeParam();
	if (vecParam.size() != 4)
	{
		printf("param error\n");
		return -1;
	}

	//��Ƶ�ļ�·��
	const char* video = vecParam[1].c_str();
	//�洢·��
	const char* pathFormat = vecParam[2].c_str();
	//ͼƬ��ʼ���
	int frame = atoi(vecParam[3].c_str()) - 1;
	CvCapture* cap = cvCaptureFromFile(video);
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
		spSaveTask->setParam(CStringManager::Format(pathFormat, ++frame), cvCloneImage(pImg));
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