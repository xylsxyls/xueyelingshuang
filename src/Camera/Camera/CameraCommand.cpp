#include "CameraCommand.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CameraResult.h"

extern int32_t g_frameApplySendPid;
extern int32_t g_frameAbandonSendPid;
extern int32_t g_openSendPid;
extern int32_t g_closeSendPid;
extern int32_t g_command;
extern Semaphore g_semaphore;
extern int32_t g_cameraId;
extern int32_t g_width;
extern int32_t g_height;

void CameraCommand::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	std::string message(buffer, length);
	std::vector<std::string> vecParam = CStringManager::split(message, ",");
	if (vecParam.empty())
	{
		return;
	}
	if (vecParam[0] == "OpenCamera")
	{
		if (vecParam.size() != 4)
		{
			return;
		}
		g_openSendPid = sendPid;
		g_command = CAMERA_COMMAND_OPEN_CAMERA;
		g_cameraId = atoi(vecParam[1].c_str());
		g_width = atoi(vecParam[2].c_str());
		g_height = atoi(vecParam[3].c_str());
		g_semaphore.signal();
	}
	else if (vecParam[0] == "CloseCamera")
	{
		g_closeSendPid = sendPid;
		g_command = CAMERA_COMMAND_CLOSE_CAMERA;
		g_semaphore.signal();
	}
	else if (vecParam[0] == "FrameApply")
	{
		g_frameApplySendPid = sendPid;
		g_command = CAMERA_COMMAND_FRAME_APPLY;
		g_semaphore.signal();
	}
	else if (vecParam[0] == "FrameAbandon")
	{
		g_frameAbandonSendPid = sendPid;
		g_command = CAMERA_COMMAND_FRAME_ABANDON;
		g_semaphore.signal();
	}
	else if (vecParam[0] == "IsOpen")
	{
		g_command = CAMERA_COMMAND_IS_OPEN;
		g_semaphore.signal();
	}
	else if (vecParam[0] == "Exit")
	{
		g_command = CAMERA_COMMAND_EXIT;
		g_semaphore.signal();
	}
}