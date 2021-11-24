#include "stdafx.h"
#include "CameraCallback.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CameraTestDlg.h"

CameraCallback::CameraCallback():
m_dlg(nullptr)
{

}

void CameraCallback::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	std::string message(buffer, length);
	std::vector<std::string> vecParam = CStringManager::split(message, ",");
	if (vecParam.empty())
	{
		return;
	}
	if (vecParam[0] == "Frame")
	{
		if (vecParam.size() != 3)
		{
			return;
		}
		std::unique_lock<std::mutex> lock(m_dlg->m_frameMutex);
		if (vecParam[1] == "0")
		{
			m_dlg->m_frame = vecParam[2];
		}
		else
		{
			m_dlg->m_frameIr = vecParam[2];
		}
	}
}

void CameraCallback::setDlg(CCameraTestDlg* dlg)
{
	m_dlg = dlg;
}