#include "stdafx.h"
#include "CMouseConfig.h"
#include "CMouse/CMouseAPI.h"

CMouseConfig::CMouseConfig() :
m_wParam(0),
m_lParam(0),
m_leftDown(false),
m_leftUp(false),
m_rightDown(false),
m_rightUp(false),
m_leftHasDown(false),
m_rightHasDown(false)
{

}

CMouseConfig& CMouseConfig::instance()
{
	static CMouseConfig s_cmouseConfig;
	return s_cmouseConfig;
}

void CMouseConfig::acceptParam(WPARAM wParam, LPARAM lParam)
{
	m_leftDown = false;
	m_leftUp = false;
	m_rightDown = false;
	m_rightUp = false;

	if (wParam == 513)
	{
		if (!m_leftHasDown)
		{
			m_leftDown = true;
			m_leftHasDown = true;
			m_leftWatch.SetWatchTime(0);
		}
	}
	else if (wParam == 514)
	{
		if (m_leftHasDown)
		{
			m_leftUp = true;
			m_leftHasDown = false;
		}
	}
	else if (wParam == 516)
	{
		if (!m_rightHasDown)
		{
			m_rightDown = true;
			m_rightHasDown = true;
			m_rightWatch.SetWatchTime(0);
		}
	}
	else if (wParam == 517)
	{
		if (m_rightHasDown)
		{
			m_rightUp = true;
			m_rightHasDown = false;
		}
	}
	else if (wParam == 512)
	{
		m_isMove = true;
	}

	m_currentPos = CMouse::GetCurrentPos();
}

