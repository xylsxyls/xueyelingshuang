#include "CMouseConfig.h"
#include "CHook/CHookAPI.h"

CMouseConfig::CMouseConfig() :
m_wParam(0),
m_lParam(0),
m_leftDown(false),
m_leftUp(false),
m_rightDown(false),
m_rightUp(false),
m_midDown(false),
m_midUp(false),
m_leftHasDown(false),
m_rightHasDown(false),
m_midHasDown(false)
{

}

CMouseConfig& CMouseConfig::instance()
{
	static CMouseConfig s_cmouseConfig;
	return s_cmouseConfig;
}

void CMouseConfig::acceptParam(WPARAM wParam, LPARAM lParam)
{
	m_wParam = wParam;
	m_lParam = lParam;

	m_leftDown = false;
	m_leftUp = false;
	m_rightDown = false;
	m_rightUp = false;
	m_midDown = false;
	m_midUp = false;
	m_isMove = false;

	if (wParam == WM_LBUTTONDOWN)
	{
		if (!m_leftHasDown)
		{
			m_leftDown = true;
			m_leftHasDown = true;
			m_leftWatch.SetWatchTime(0);
		}
	}
	else if (wParam == WM_LBUTTONUP)
	{
		if (m_leftHasDown)
		{
			m_leftUp = true;
			m_leftHasDown = false;
		}
	}
	else if (wParam == WM_RBUTTONDOWN)
	{
		if (!m_rightHasDown)
		{
			m_rightDown = true;
			m_rightHasDown = true;
			m_rightWatch.SetWatchTime(0);
		}
	}
	else if (wParam == WM_RBUTTONUP)
	{
		if (m_rightHasDown)
		{
			m_rightUp = true;
			m_rightHasDown = false;
		}
	}
	else if (wParam == WM_MBUTTONDOWN)
	{
		if (!m_midHasDown)
		{
			m_midDown = true;
			m_midHasDown = true;
			m_midWatch.SetWatchTime(0);
		}
	}
	else if (wParam == WM_MBUTTONUP)
	{
		if (m_midHasDown)
		{
			m_midUp = true;
			m_midHasDown = false;
		}
	}
	else if (wParam == WM_MOUSEMOVE)
	{
		m_isMove = true;
	}

	m_currentPos = CHook::GetMousePosition(lParam);
}