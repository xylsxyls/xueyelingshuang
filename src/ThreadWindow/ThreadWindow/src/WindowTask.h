#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

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
	void StopTask();

protected:
	LRESULT (CALLBACK* m_wndProc)(HWND, UINT, WPARAM, LPARAM);
	bool m_visible;
	HWND* m_hWnd;
	bool m_exit;
	bool m_hasExit;
};