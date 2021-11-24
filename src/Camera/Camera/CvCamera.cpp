#ifdef CAMERA_USE_OPENCV
#include "CvCamera.h"
//#include "LogManager/LogManagerAPI.h"
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <Windows.h>
#include <dshow.h>
#include "D:\\SendToMessageTest.h"

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

CvCamera::CvCamera() :
	m_capture(nullptr),
	m_isOpen(false)
{

}

CvCamera::~CvCamera()
{
	if (m_isOpen)
	{
		closeCamera();
	}
}

bool CvCamera::openCamera(int32_t cameraId, char c1, char c2, char c3, char c4)
{
	if (m_isOpen)
	{
		return false;
	}
	m_capture = new cv::VideoCapture(cameraId);
	if (m_capture == nullptr)
	{
		return false;
	}
	m_capture->set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc(c1, c2, c3, c4));
	m_isOpen = true;
	return true;
}

bool CvCamera::setResolution(int32_t width, int32_t height)
{
	if (!m_isOpen)
	{
		return false;
	}
	m_capture->set(cv::CAP_PROP_FRAME_WIDTH, width);
	m_capture->set(cv::CAP_PROP_FRAME_HEIGHT, height);
	return true;
}

void CvCamera::closeCamera()
{
	if (m_isOpen)
	{
		delete m_capture;
		m_capture = nullptr;
		m_isOpen = false;
	}
}

bool CvCamera::grab(cv::Mat& frame)
{
	if (!m_isOpen)
	{
		return false;
	}

	//读取当前帧
	*m_capture >> frame;

	//若视频完成播放，退出循环
	if (frame.empty())
	{
		return false;
	}
	return true;
}
#endif