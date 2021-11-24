#include "CameraGrabTask.h"
#include "Camera.h"
#include "CBase64/CBase64API.h"
#include "ProcessWork/ProcessWorkAPI.h"

extern std::vector<int32_t> g_allSendPid;
extern std::mutex g_allSendPidMutex;
extern int32_t g_cameraId;

CameraGrabTask::CameraGrabTask():
m_camera(nullptr),
m_exit(false)
{

}

void CameraGrabTask::DoTask()
{
	while (!m_exit)
	{
		std::vector<unsigned char> frame;
		m_camera->grab(frame);
		std::string strFrame = CBase64::encode((char*)frame.data(), frame.size());
		{
			std::unique_lock<std::mutex> lock(g_allSendPidMutex);
			int32_t index = -1;
			while (index++ != g_allSendPid.size() - 1)
			{
				ProcessWork::instance().send(g_allSendPid[index], "Frame," + std::to_string(g_cameraId) + "," + strFrame);
			}
		}
		
	}
	std::unique_lock<std::mutex> lock(g_allSendPidMutex);
	int32_t index = -1;
	while (index++ != g_allSendPid.size() - 1)
	{
		ProcessWork::instance().send(g_allSendPid[index], "Frame," + std::to_string(g_cameraId) + ",");
	}
}

void CameraGrabTask::StopTask()
{
	m_exit = true;
}

void CameraGrabTask::setParam(Camera* camera)
{
	m_camera = camera;
}