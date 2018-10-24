#include "WindowTask.h"

WindowTask::WindowTask():
m_wndProc(nullptr),
m_visible(false),
m_exit(false),
m_hasExit(false),
m_hWnd(nullptr)
{

}

void WindowTask::DoTask()
{
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = m_wndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(NULL); // m_instance;
	wcex.hIcon = 0;//LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
	wcex.hCursor = 0;//LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = 0;//(HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = 0;//MAKEINTRESOURCE(IDC_WIN32PROJECT1);
	wcex.lpszClassName = "Win32ThreadWindow";//szWindowClass;
	wcex.hIconSm = 0;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	::RegisterClassEx(&wcex);

	*m_hWnd = ::CreateWindow(wcex.lpszClassName, wcex.lpszClassName, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, wcex.hInstance, NULL);
	
	if (m_visible)
	{
		::ShowWindow(*m_hWnd, SW_SHOW);
		::UpdateWindow(*m_hWnd);
	}

	//主消息循环，执行了代码在这里堵住
	MSG msg;
	while (!m_exit && ::GetMessage(&msg, NULL, 0, 0))
	{
		if (!::TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}
	m_hasExit = true;
}

void WindowTask::setHwnd(HWND* hWnd)
{
	m_hWnd = hWnd;
}

void WindowTask::StopTask()
{
	m_exit = true;
	while (!m_hasExit)
	{
		::SendMessage(*m_hWnd, WM_QUIT, NULL, NULL);
		Sleep(1);
	}
}

void WindowTask::setCallback(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM))
{
	m_wndProc = wndProc;
}

void WindowTask::setVisible(bool visible)
{
	m_visible = visible;
}