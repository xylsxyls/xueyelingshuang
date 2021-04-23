#include "WindowTask.h"
#include "Semaphore/SemaphoreAPI.h"

WindowTask::WindowTask():
m_wndProc(nullptr),
m_visible(false),
m_exit(false),
m_hWnd(nullptr),
m_semaphore(nullptr),
m_hasTitle(false)
{
	m_rect.left = 0;
	m_rect.top = 0;
	m_rect.right = 0;
	m_rect.bottom = 0;
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
	wcex.lpszClassName = m_windowName.c_str();//szWindowClass;
	wcex.hIconSm = 0;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	::RegisterClassEx(&wcex);

	*m_hWnd = ::CreateWindow(wcex.lpszClassName, wcex.lpszClassName, m_hasTitle ? WS_OVERLAPPEDWINDOW :
		WS_OVERLAPPED |
		WS_SYSMENU | 
		WS_MINIMIZEBOX | 
		WS_MAXIMIZEBOX |
		WS_POPUP,
		m_rect.left, m_rect.top, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, NULL, NULL, wcex.hInstance, NULL);
	//加载系统自带鼠标样式
	HCURSOR hCursor = ::LoadCursor(NULL, IDC_ARROW);
#ifdef _WIN64
	::SetClassLongPtr(*m_hWnd, GCLP_HCURSOR, (LONG_PTR)hCursor);
#else
	::SetClassLong(*m_hWnd, GCL_HCURSOR, (LONG)hCursor);
#endif

	m_semaphore->signal();
	
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
}

void WindowTask::setHwnd(HWND* hWnd)
{
	m_hWnd = hWnd;
}

void WindowTask::setSemaphore(Semaphore* semaphore)
{
	m_semaphore = semaphore;
}

void WindowTask::setWindowPos(const RECT& rect)
{
	m_rect = rect;
}

void WindowTask::setHasTitle(bool hasTitle)
{
	m_hasTitle = hasTitle;
}

void WindowTask::setWindowName(const std::string& windowName)
{
	m_windowName = windowName;
}

void WindowTask::StopTask()
{
	m_exit = true;
	::SendMessage(*m_hWnd, WM_QUIT, NULL, NULL);
}

void WindowTask::setCallback(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM))
{
	m_wndProc = wndProc;
}

void WindowTask::setVisible(bool visible)
{
	m_visible = visible;
}