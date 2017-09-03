#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <windows.h>

class CEncodeDecodeTask : public CTask
{
public:
	CEncodeDecodeTask(int32_t taskId, const std::string& path, HWND hwnd, bool isEncode);
	virtual void DoTask();
	virtual void StopTask(bool ifChoke);
	virtual bool ReExecute();
	virtual CTask* Clone();
	virtual bool HasExecuted();

private:
	std::atomic<bool> m_hasExitSignal = false;
	std::atomic<bool> m_hasExecuted = false;

	std::string m_path;
	HWND m_hWnd = nullptr;
	bool m_isEncode;
};