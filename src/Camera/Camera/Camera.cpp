#include "Camera.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif
#include "CameraDS.h"
#include "CvCamera.h"
#include <opencv2/core/core_c.h>
//#include "LogManager/LogManagerAPI.h"
//#include "D:\\LogSender\\LogSenderAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CameraCommand.h"
#include "CameraGrabTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CameraResult.h"
#include "CameraCheckTask.h"

#ifdef _DEBUG
#pragma comment(lib, "opencv_world320d.lib")
#else
#pragma comment(lib, "opencv_world320.lib")
#endif

Camera::Camera():
m_cvCamera(nullptr),
m_dsCamera(nullptr)
{
#ifdef CAMERA_USE_DIRECTSHOW
	m_dsCamera = new CCameraDS;
#elif CAMERA_USE_OPENCV
	m_cvCamera = new CvCamera;
#endif
}

Camera::~Camera()
{
#ifdef CAMERA_USE_DIRECTSHOW
	delete m_dsCamera;
	m_dsCamera = nullptr;
#elif CAMERA_USE_OPENCV
	delete m_cvCamera;
	m_cvCamera = nullptr;
#endif
}

void Camera::comInit()
{
	CCameraDS::ComInit();
}

void Camera::comUninit()
{
	CCameraDS::ComUninit();
}

bool Camera::openCamera(int32_t cameraId, int32_t width, int32_t height)
{
	bool result = false;
#ifdef CAMERA_USE_DIRECTSHOW
	result = m_dsCamera->OpenCamera(cameraId, false, width, height);
#elif CAMERA_USE_OPENCV
	result = m_cvCamera->openCamera(cameraId);
	m_cvCamera->setResolution(width, height);
#endif
	return result;
}

void Camera::closeCamera()
{
#ifdef CAMERA_USE_DIRECTSHOW
	m_dsCamera->CloseCamera();
#elif CAMERA_USE_OPENCV
	m_cvCamera->closeCamera();
#endif
}

bool MatToJPEGByteArray(std::vector<unsigned char>& buff, const cv::Mat& mat, int32_t ratio)
{
	if (mat.empty())
	{
		return false;
	}
	std::vector<int> param = std::vector<int>(2);
	param[0] = cv::IMWRITE_JPEG_QUALITY;
	param[1] = ratio; // default(95) 0-100
	return cv::imencode(".jpg", mat, buff, param);
}

bool Camera::grab(std::vector<unsigned char>& frame)
{
#ifdef CAMERA_USE_DIRECTSHOW
	IplImage* iplimage = m_dsCamera->QueryFrame();
	if (iplimage == nullptr)
	{
		LOG_SEND_ONLY_ERROR("iplimage = nullptr");
		return false;
	}
	cv::Mat matFrame = cv::cvarrToMat(iplimage);
	if (matFrame.empty())
	{
		LOG_SEND_ONLY_ERROR("matFrame empty");
		return false;
	}
	if (!MatToJPEGByteArray(frame, matFrame, 75))
	{
		LOG_SEND_ONLY_ERROR("MatToJPEGByteArray error");
		return false;
	}
#elif CAMERA_USE_OPENCV
	cv::Mat matFrame;
	if (!m_cvCamera->grab(matFrame))
	{
		return false;
	}
	if (!MatToJPEGByteArray(frame, matFrame))
	{
		return false;
	}
#endif
	return true;
}

bool Camera::isOpen(int32_t cameraId)
{
#ifdef CAMERA_USE_DIRECTSHOW
	CCameraDS dsCamera;
	if (!dsCamera.OpenCamera(cameraId, false))
	{
		return true;
	}
	if (dsCamera.QueryFrame() == nullptr)
	{
		dsCamera.CloseCamera();
		return true;
	}
	dsCamera.CloseCamera();
	return false;
#elif CAMERA_USE_OPENCV
	CvCamera cvCamera;
	if (!cvCamera.openCamera(cameraId))
	{
		return true;
	}
	cv::Mat frame;
	if (!cvCamera.grab(frame))
	{
		cvCamera.closeCamera();
		return true;
	}
	cvCamera.closeCamera();
	return false;
#endif
}

uint32_t g_grabThreadId = 0;
uint32_t g_checkThreadId = 0;
int32_t g_openSendPid = 0;
int32_t g_closeSendPid = 0;
int32_t g_frameApplySendPid = 0;
int32_t g_frameAbandonSendPid = 0;
std::mutex g_allSendPidMutex;
std::vector<int32_t> g_allSendPid;
//0未打开，1已经打开，2正在打开，3正在关闭
int32_t g_cameraState = CAMERA_HAS_NOT_OPEN;
int32_t g_command = -1;
Semaphore g_semaphore;
int32_t g_cameraId = 0;
int32_t g_width = 0;
int32_t g_height = 0;

Camera camera;

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		if (g_cameraState == CAMERA_OPEN_ING)
		{
			while (g_cameraState == CAMERA_OPEN_ING)
			{
				CSystem::Sleep(50);
			}
		}
		if (g_cameraState == CAMERA_HAS_OPEN)
		{
			CTaskThreadManager::Instance().Uninit(g_grabThreadId);
			g_grabThreadId = 0;
			camera.closeCamera();
		}
		if (g_grabThreadId != 0)
		{
			CTaskThreadManager::Instance().Uninit(g_grabThreadId);
		}
		CTaskThreadManager::Instance().Uninit(g_checkThreadId);
		ProcessWork::instance().uninitReceive();
		Camera::comUninit();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

#elif __unix__
//ctrl+c消息捕获函数
void CtrlCMessage(int eve)
{
	if (eve == 2)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		CTaskThreadManager::Instance().Uninit(g_grabThreadId);
		ProcessWork::instance().uninitReceive();
		exit(0);
	}
}

struct CtrlC
{
	CtrlC()
	{
		struct sigaction sigIntHandler;
		sigIntHandler.sa_handler = CtrlCMessage;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;
		sigaction(SIGINT, &sigIntHandler, nullptr);
	}
};

CtrlC g_ctrlc;
#endif

int32_t main()
{
	g_checkThreadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<CameraCheckTask> spCameraCheckTask(new CameraCheckTask);
	CTaskThreadManager::Instance().GetThreadInterface(g_checkThreadId)->PostTask(spCameraCheckTask);
	g_grabThreadId = CTaskThreadManager::Instance().Init();
	CameraCommand cameraCommand;
	ProcessWork::instance().addProcessReceiveCallback(&cameraCommand);
	auto beginTime = CSystem::GetHighTickCount();
	ProcessWork::instance().initReceive();
	printf("%d", CSystem::GetHighTickCountMicroRunTime(beginTime));
	Camera::comInit();
	while (true)
	{
		g_semaphore.wait();
		switch (g_command)
		{
		case CAMERA_COMMAND_OPEN_CAMERA:
		{
			if (g_cameraState == CAMERA_HAS_OPEN)
			{
				bool isFind = false;
				{
					std::unique_lock<std::mutex> lock(g_allSendPidMutex);
					isFind = std::find(g_allSendPid.begin(), g_allSendPid.end(), g_openSendPid) != g_allSendPid.end();
					if (!isFind)
					{
						g_allSendPid.push_back(g_openSendPid);
					}
				}
				ProcessWork::instance().send(g_openSendPid, "OpenCamera," + std::to_string(isFind ? g_cameraState : CAMERA_OK));
				break;
			}
			if (g_cameraState != CAMERA_HAS_NOT_OPEN)
			{
				ProcessWork::instance().send(g_openSendPid, "OpenCamera," + std::to_string(g_cameraState));
				break;
			}
			g_cameraState = CAMERA_OPEN_ING;
			bool openResult = camera.openCamera(g_cameraId, g_width, g_height);
			if (!openResult)
			{
				g_cameraState = CAMERA_HAS_NOT_OPEN;
				ProcessWork::instance().send(g_openSendPid, "OpenCamera," + std::to_string(CAMERA_OPEN_ERROR));
				break;
			}
			std::vector<unsigned char> frame;
			bool grabResult = camera.grab(frame);
			if (!grabResult)
			{
				g_cameraState = CAMERA_HAS_NOT_OPEN;
				ProcessWork::instance().send(g_openSendPid, "OpenCamera," + std::to_string(CAMERA_GRAB_ERROR));
				break;
			}

			std::shared_ptr<CameraGrabTask> spCameraGrabTask(new CameraGrabTask);
			spCameraGrabTask->setParam(&camera);
			CTaskThreadManager::Instance().GetThreadInterface(g_grabThreadId)->PostTask(spCameraGrabTask);
			g_cameraState = CAMERA_HAS_OPEN;
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				g_allSendPid.push_back(g_openSendPid);
			}
			ProcessWork::instance().send(g_openSendPid, "OpenCamera," + std::to_string(CAMERA_OK));
		}
		break;
		case CAMERA_COMMAND_CLOSE_CAMERA:
		{
			if (g_cameraState != CAMERA_HAS_OPEN)
			{
				if (g_closeSendPid != 0)
				{
					ProcessWork::instance().send(g_closeSendPid, "CloseCamera," + std::to_string(g_cameraState));
				}
				break;
			}
			bool isEmpty = false;
			uint32_t closeSendPid = g_closeSendPid;
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				auto itSendPid = std::find(g_allSendPid.begin(), g_allSendPid.end(), g_closeSendPid);
				if (itSendPid != g_allSendPid.end())
				{
					g_allSendPid.erase(itSendPid);
				}
				isEmpty = g_allSendPid.empty();
			}
			
			if (!isEmpty)
			{
				ProcessWork::instance().send(closeSendPid, "Frame," + std::to_string(g_cameraId) + ",");
				ProcessWork::instance().send(closeSendPid, "CloseCamera," + std::to_string(CAMERA_OK));
				break;
			}
			g_cameraState = CAMERA_CLOSE_ING;
			CTaskThreadManager::Instance().Uninit(g_grabThreadId);
			g_grabThreadId = CTaskThreadManager::Instance().Init();
			camera.closeCamera();
			g_cameraState = CAMERA_HAS_NOT_OPEN;
			if (closeSendPid != 0)
			{
				ProcessWork::instance().send(closeSendPid, "CloseCamera," + std::to_string(CAMERA_OK));
			}
		}
		break;
		case CAMERA_COMMAND_FRAME_APPLY:
		{
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				if (std::find(g_allSendPid.begin(), g_allSendPid.end(), g_frameApplySendPid) == g_allSendPid.end())
				{
					g_allSendPid.push_back(g_frameApplySendPid);
				}
			}
			ProcessWork::instance().send(g_frameApplySendPid, "FrameApply," + std::to_string(CAMERA_OK));
		}
		break;
		case CAMERA_COMMAND_FRAME_ABANDON:
		{
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				auto it = std::find(g_allSendPid.begin(), g_allSendPid.end(), g_frameAbandonSendPid);
				if (it != g_allSendPid.end())
				{
					g_allSendPid.erase(it);
				}
			}
			ProcessWork::instance().send(g_frameAbandonSendPid, "Frame," + std::to_string(g_cameraId) + ",");
			ProcessWork::instance().send(g_frameAbandonSendPid, "FrameAbandon," + std::to_string(CAMERA_OK));
		}
		break;
		case CAMERA_COMMAND_IS_OPEN:
		{
			bool isOpen = Camera::isOpen(g_cameraId);
			ProcessWork::instance().send(g_openSendPid, "IsOpen," + std::to_string(isOpen));
		}
		break;
		case CAMERA_COMMAND_EXIT:
		{
			if (g_cameraState == CAMERA_OPEN_ING)
			{
				while (g_cameraState == CAMERA_OPEN_ING)
				{
					CSystem::Sleep(50);
				}
			}
			if (g_cameraState == CAMERA_HAS_OPEN)
			{
				CTaskThreadManager::Instance().Uninit(g_grabThreadId);
				g_grabThreadId = 0;
				camera.closeCamera();
			}
			if (g_grabThreadId != 0)
			{
				CTaskThreadManager::Instance().Uninit(g_grabThreadId);
			}
			CTaskThreadManager::Instance().Uninit(g_checkThreadId);
			ProcessWork::instance().uninitReceive();
			Camera::comUninit();
			return 0;
		}
		break;
		default:
			break;
		}
		g_command = -1;
	}
	getchar();
	return 0;
}