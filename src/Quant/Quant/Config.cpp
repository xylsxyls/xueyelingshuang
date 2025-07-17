#include "Config.h"
#include "CSystem/CSystemAPI.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#include <X11/Xlib.h>
#endif

CConfig::CConfig()
{
#ifdef _MSC_VER
	m_screenPixel = GetSystemMetrics(SM_CXSCREEN) * GetSystemMetrics(SM_CYSCREEN);
#else
	Display* display = XOpenDisplay(nullptr);
	if (display)
	{
		int screen = DefaultScreen(display);
		m_screenPixel = DisplayWidth(display, screen) * DisplayHeight(display, screen);
		XCloseDisplay(display);
	}
#endif
	m_currentExePath = CSystem::GetCurrentExePath();
	m_computerName = CSystem::getComputerName();
}

CConfig& CConfig::instance()
{
	static CConfig s_cconfig;
	return s_cconfig;
}