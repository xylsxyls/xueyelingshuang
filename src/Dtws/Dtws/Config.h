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

class Dtws;

struct CConfig
{
	std::vector<xyls::Point> m_accountPoint;
	std::vector<xyls::Rect> m_rightTopRect;
	std::vector<xyls::Point> m_clickTop;
	std::vector<xyls::Point> m_talkPoint;
	std::vector<xyls::Rect> m_talkheadRect;
	std::vector<xyls::Rect> m_chatRect;
	std::vector<xyls::Rect> m_bloodRect;
	std::vector<xyls::Point> m_taskPoint;
	std::vector<xyls::Point> m_intoGamePoint;
	xyls::Point m_accept = { 63, 418 };
	xyls::Point m_get = { 64, 326 };
	int32_t m_accountCount = 1;
	uint32_t* m_taskThreadId = nullptr;
	uint32_t* m_threadId = nullptr;
	CStopWatch m_stopWatch;
	bool m_altDown = false;
	bool m_ctrlDown = false;
	bool m_muqing = true;
	bool m_hook = true;
	bool m_isBigLache = true;
	Dtws* m_dtws = nullptr;

	CStopWatch m_textWatch;
	uint32_t m_textWatchTime = 10000;

	bool m_isPrintVkCode = false;

	std::string m_currentExePath;

protected:
	CConfig();

public:
	static CConfig& instance();
};