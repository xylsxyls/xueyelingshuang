#include "ThreadWindow.h"
#include "WindowTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

ThreadWindow::ThreadWindow():
m_threadId(0),
m_hWnd(nullptr),
m_hdc(nullptr)
{

}

ThreadWindow::~ThreadWindow()
{
	Destroy();
}

HWND ThreadWindow::Create(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
	bool visible,
	int32_t left,
	int32_t top,
	int32_t width,
	int32_t height,
	bool hasTitle,
	const std::string& windowName)
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
	task->setHwnd(&m_hWnd);
	task->setSemaphore(&m_semaphore);
	task->setWindowPos(RECT{ left, top, left + width, top + height });
	task->setHasTitle(hasTitle);
	task->setWindowName(windowName);
	spTask.reset(task);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spTask);
	
	m_semaphore.wait();
	return m_hWnd;
}

HWND ThreadWindow::GetHWND()
{
	return m_hWnd;
}

HDC ThreadWindow::GetHDC()
{
	if (m_hdc == nullptr)
	{
		m_hdc = ::GetDC(m_hWnd);
		return m_hdc;
	}
	return m_hdc;
}

void ThreadWindow::ReleaseHDC()
{
	if (m_hdc != nullptr)
	{
		::ReleaseDC(m_hWnd, m_hdc);
		m_hdc = nullptr;
	}
}

void ThreadWindow::Destroy()
{
	ReleaseHDC();
	if (m_threadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_threadId);
		m_threadId = 0;
	}
}