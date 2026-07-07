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
	m_iniDbPath = m_currentExePath;
	m_dbPath = m_currentExePath;
	m_cpuCoreCount = CSystem::GetCPUCoreCount();
	m_normalShares = 800;
	m_ignoreTrans = 2;
	m_stock = "300059";
	m_allStock = { "600975", "600348", "600867", "300059" };
	m_allBeginTime = 20200201;
	m_allEndTime = 30250101;
	m_initialFund = 1000000 * 100;
	m_completeTaskCount = 0;
	m_ignoreTaskCount = 0;
	m_showCount = 10;
	m_selectCount = 100000;
	m_avgCount = 10;
	m_runDayCount = 10;
}

CConfig& CConfig::instance()
{
	static CConfig s_config;
	return s_config;
}