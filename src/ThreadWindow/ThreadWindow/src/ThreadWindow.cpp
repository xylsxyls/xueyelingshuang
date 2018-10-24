#include "ThreadWindow.h"
#include "WindowTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

ThreadWindow::ThreadWindow():
m_threadId(0)
{

}

HWND ThreadWindow::Create(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM), bool visible)
{
	if (m_threadId != 0)
	{
		return nullptr;
	}
	m_threadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<WindowTask> spTask;
	WindowTask* task = new WindowTask;
	task->setCallback(wndProc);
	task->setVisible(visible);
	HWND hWnd = nullptr;
	task->setHwnd(&hWnd);
	spTask.reset(task);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spTask);
	while (hWnd == nullptr)
	{
		Sleep(1);
	}
	return hWnd;
}

void ThreadWindow::Destroy()
{
	CTaskThreadManager::Instance().Uninit(m_threadId);
	m_threadId = 0;
}