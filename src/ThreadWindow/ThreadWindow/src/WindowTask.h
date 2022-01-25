#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <windows.h>

class ThreadWindowCallback;

class WindowTask : public CTask
{
public:
	WindowTask();

public:
	void DoTask();
	void setCallback(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM));
	void setVisible(bool visible);
	void setHwnd(HWND* hWnd);
	void setSemaphore(Semaphore* semaphore);
	void setWindowPos(const RECT& rect);
	void setHasTitle(bool hasTitle);
	void setWindowName(const std::string& windowName);
	void StopTask();

protected:
	LRESULT (CALLBACK* m_wndProc)(HWND, UINT, WPARAM, LPARAM);
	bool m_visible;
	HWND* m_hWnd;
	std::atomic<bool> m_exit;
	Semaphore* m_semaphore;
	RECT m_rect;
	bool m_hasTitle;
	std::string m_windowName;
};