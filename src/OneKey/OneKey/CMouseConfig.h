#pragma once
#include <stdint.h>
#include <windows.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "Point/PointAPI.h"

#define g_mouse CMouseConfig::instance()

struct CMouseConfig
{
	int32_t m_wParam;
	int32_t m_lParam;

	xyls::Point m_currentPos;

	bool m_leftDown;
	bool m_leftUp;
	bool m_rightDown;
	bool m_rightUp;
	bool m_isMove;

	bool m_leftHasDown;
	bool m_rightHasDown;

	CStopWatch m_leftWatch;
	CStopWatch m_rightWatch;

protected:
	CMouseConfig();

public:
	static CMouseConfig& instance();

public:
	void acceptParam(WPARAM wParam, LPARAM lParam);
};