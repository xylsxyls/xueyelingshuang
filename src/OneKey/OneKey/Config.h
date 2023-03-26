#pragma once
#include <stdint.h>
#include <windows.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "Point/PointAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

#define g_config CConfig::instance()

struct CConfig
{
	HWND m_hWnd = nullptr;
	HWND m_editWnd = nullptr;

	CStopWatch m_stopWatch;
	CStopWatch m_textWatch;
	CStopWatch m_moveWatch;
	CStopWatch m_equipWatch;

	int32_t m_type = 1;

	bool m_hasMove = false;
	bool m_qKey = false;
	bool m_checkHero = false;

	int32_t m_screenWidth = 3840;
	int32_t m_screenHeight = 2160;

	int32_t m_gameScreenHeight = 1646;
	int32_t m_gameScreenMargin = 10;

	xyls::Point m_heroHeadPoint[5];
	//xyls::Point g_heroHeadPoint[5] = { { 1151, 739 }, { 1192, 620 }, { 1266, 514 }, { 1367, 437 }, { 1483, 383 } };
	int32_t m_side = 150;

	xyls::Point m_heroHeadShowPoint = { 817, 468 };
	int32_t m_showSide = 75;
	int32_t m_heroHeadSpace = 40;

	xyls::Point m_editHeroPoint = { 2000, 1000 };

	int32_t m_heroHeadTime = 1500;
	int32_t m_codetimes = 0;

	xyls::Point m_equipBeginPoint = { 1384, 1699 };
	int32_t m_equipWidth = 238;

	xyls::Point m_wCenterPoint = { 3020, 1398 };
	xyls::Point m_rCenterPoint = { 3323, 1219 };
	xyls::Point m_fCenterPoint = { 2584, 1736 };
	xyls::Point m_bCenterPoint = { 2136, 1735 };

	xyls::Point m_heroCenterPoint = { 3840 / 2, 1236 };

	bool m_clickMap = false;
	bool m_moveUse = false;
	bool m_fullLast = false;
	bool m_holdW = false;
	bool m_holdR = false;
	bool m_info = false;

	bool m_hasF = false;
	bool m_hasS = false;

	int32_t m_code1 = 'H';
	int32_t m_code2 = 'C';
	bool m_isMain = false;

	CStopWatch m_code2Watch;

	std::atomic<bool> m_rightMouse = true;

	uint32_t m_threadId = 0;

	std::shared_ptr<CTaskThread> m_taskThread;

	bool m_isPrintVkCode = true;

	uint32_t m_textWatchTime = 10000;

	int32_t m_msTimerId = 1000;
	int32_t m_msTime = 10;

	std::vector<std::string> m_heroName;

	bool m_blxca = false;
	bool m_blxcRight = false;

	uint32_t m_rightClickTime = 2000;

	xyls::Point m_speakBeginPoint = { 3479, 910 };
	xyls::Point m_speakEndPoint = { 3046, 1535 };

protected:
	CConfig();

public:
	static CConfig& instance();

	int32_t nameType(const std::string& name);

	bool timerMs(int32_t timeMs, int32_t msTimes);
};

//struct CConfig
//{
//	HWND m_hWnd = nullptr;
//	HWND m_editWnd = nullptr;
//
//	CStopWatch m_stopWatch;
//	CStopWatch m_textWatch;
//	CStopWatch m_moveWatch;
//	CStopWatch m_equipWatch;
//
//	int32_t m_type = 1;
//
//	bool m_hasMove = false;
//	bool m_qKey = false;
//	bool m_checkHero = false;
//
//	xyls::Point m_heroHeadPoint[5];
//	//xyls::Point g_heroHeadPoint[5] = { { 1151, 739 }, { 1192, 620 }, { 1266, 514 }, { 1367, 437 }, { 1483, 383 } };
//	int32_t m_side = 75;
//
//	xyls::Point m_heroHeadShowPoint = { 407, 232 }; 817, 468
//	int32_t m_showSide = 40;
//	int32_t m_heroHeadSpace = 20;
//
//	xyls::Point m_editHeroPoint = { 1000, 500 };
//
//	int32_t m_heroHeadTime = 1500;
//	int32_t m_codetimes = 0;
//
//	xyls::Point m_equipBeginPoint = { 612, 952 };
//	int32_t m_equipWidth = 160;
//
//	xyls::Point m_wCenterPoint = { 1542, 749 };
//	xyls::Point m_rCenterPoint = { 1663, 609 };
//	xyls::Point m_fCenterPoint = { 1292, 869 };
//	xyls::Point m_bCenterPoint = { 1061, 860 };
//
//	xyls::Point m_heroCenterPoint = { 1920 / 2, 618 };
//
//	bool m_clickMap = false;
//	bool m_moveUse = false;
//	bool m_fullLast = false;
//	bool m_holdW = false;
//	bool m_holdR = false;
//	bool m_info = false;
//
//	bool m_hasF = false;
//	bool m_hasS = false;
//
//	int32_t m_code1 = 'H';
//	int32_t m_code2 = 'C';
//	bool m_isMain = false;
//
//	CStopWatch m_code2Watch;
//
//	std::atomic<bool> m_rightMouse = true;
//
//	uint32_t m_threadId = 0;
//
//	std::shared_ptr<CTaskThread> m_taskThread;
//
//	bool m_isPrintVkCode = true;
//
//	uint32_t m_textWatchTime = 10000;
//
//	int32_t m_msTimerId = 1000;
//	int32_t m_msTime = 10;
//
//	std::vector<std::string> m_heroName;
//
//	bool m_blxca = false;
//	bool m_blxcRight = false;
//
//	uint32_t m_rightClickTime = 2000;
//
//	xyls::Point m_speakBeginPoint = { 1740, 448 };
//	xyls::Point m_speakEndPoint = { 1517, 764 };
//
//protected:
//	CConfig();
//
//public:
//	static CConfig& instance();
//
//	int32_t nameType(const std::string& name);
//
//	bool timerMs(int32_t timeMs, int32_t msTimes);
//};