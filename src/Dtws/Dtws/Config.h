#pragma once
#include <stdint.h>
#include <windows.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "Point/PointAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"

#define g_config CConfig::instance()
#define g_screen g_config.m_accountConfig[g_config.m_screenPixel][g_config.m_accountCount]

class Dtws;

struct AccountConfig
{
	std::vector<xyls::Point> m_accountPoint;
	std::vector<xyls::Rect> m_rightTopRect;
	std::vector<xyls::Point> m_clickTop;
	std::vector<xyls::Point> m_talkHeroHeadPoint;
	std::vector<xyls::Rect> m_talkWindowHeadRect;
	std::string m_talkWindowHeadPath;
	std::string m_goWindowHeadPath;
	std::string m_chatPath;
	std::vector<xyls::Rect> m_chatRect;
	std::string m_bloodPath;
	std::vector<xyls::Rect> m_bloodRect;
	std::vector<xyls::Point> m_submitPoint;
	std::vector<xyls::Point> m_intoGamePoint;
	std::vector<xyls::Point> m_goPoint;
	std::vector<int32_t> m_goSleepTime;
};

struct CConfig
{
	std::map<int32_t, std::map<int32_t, AccountConfig>> m_accountConfig;
	
	xyls::Point m_accept = { 63, 418 };
	xyls::Point m_get = { 64, 326 };
	int32_t m_accountCount = 1;
	int32_t m_screenPixel = 1920 * 1080;
	uint32_t* m_taskThreadId = nullptr;
	uint32_t* m_threadId = nullptr;
	CStopWatch m_stopWatch;
	bool m_muqing = true;
	bool m_hook = true;
	bool m_isBigLache = true;
	Dtws* m_dtws = nullptr;

	CStopWatch m_textWatch;
	uint32_t m_textWatchTime = 10000;

	bool m_isPrintVkCode = false;

	std::string m_currentExePath;
	
	int32_t m_secondFindTime = 50;

	xyls::Point m_destEditOppositePoint = { 22, 429 };
	xyls::Point m_xunOppositePoint = { 132, 433 };
	xyls::Point m_destOppositePoint = { 230, 130 };
	xyls::Point m_getOppositePoint = { 146, 336 };
	xyls::Point m_acceptOppositePoint = { 146, 428 };

	int32_t m_destHeight = 45;
	int32_t m_goFirstSleepTime = 500;

protected:
	CConfig();

public:
	static CConfig& instance();
};