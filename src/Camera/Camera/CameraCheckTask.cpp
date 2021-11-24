#include "CameraCheckTask.h"
#include "CSystem/CSystemAPI.h"
#include "CameraResult.h"
#include "Semaphore/SemaphoreAPI.h"

extern std::vector<int32_t> g_allSendPid;
extern std::mutex g_allSendPidMutex;
extern int32_t g_command;
extern Semaphore g_semaphore;

CameraCheckTask::CameraCheckTask():
m_exit(false)
{

}

void CameraCheckTask::DoTask()
{
	bool lastIsEmpty = false;
	while (!m_exit)
	{
		CSystem::Sleep(50);
		bool isEmpty = false;
		{
			std::vector<int32_t> allSendPid;
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				allSendPid = g_allSendPid;
			}
			std::vector<int32_t> erasePid;
			int32_t index = -1;
			while (index++ != allSendPid.size() - 1)
			{
				if (m_exit)
				{
					return;
				}
				if (!CSystem::processName(allSendPid[index]).empty())
				{
					continue;
				}
				erasePid.push_back(allSendPid[index]);
			}
			{
				std::unique_lock<std::mutex> lock(g_allSendPidMutex);
				int32_t index = -1;
				while (index++ != erasePid.size() - 1)
				{
					auto it = std::find(g_allSendPid.begin(), g_allSendPid.end(), erasePid[index]);
					if (it != g_allSendPid.end())
					{
						g_allSendPid.erase(it);
					}
				}
				isEmpty = g_allSendPid.empty();
			}
		}
		
		if (m_exit)
		{
			return;
		}

		if (!lastIsEmpty && isEmpty)
		{
			g_command = CAMERA_COMMAND_CLOSE_CAMERA;
			g_semaphore.signal();
		}
		lastIsEmpty = isEmpty;
	}
}

void CameraCheckTask::StopTask()
{
	m_exit = true;
}