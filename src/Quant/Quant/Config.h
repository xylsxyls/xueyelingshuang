#pragma once
#include <stdint.h>
#include <string>
#include <atomic>
#include "CStopWatch/CStopWatchAPI.h"

#define g_config CConfig::instance()

struct CConfig
{
	int32_t m_screenPixel = 1920 * 1080;
	std::string m_currentExePath;
	std::string m_computerName;
	std::string m_iniDbPath;
	std::string m_dbPath;
	int32_t m_cpuCoreCount;
	int32_t m_normalShares;
	int32_t m_ignoreTrans;
	uint32_t m_allBeginTime;
	uint32_t m_allEndTime;
	uint32_t m_initialFund;
	CStopWatch m_time;
	std::atomic<uint32_t> m_completeTaskCount;
	std::atomic<uint32_t> m_ignoreTaskCount;
	uint32_t m_showCount;
	// 入围名次
	uint32_t m_selectCount;
	// 切割平均总个数
	uint32_t m_avgCount;
	// 模拟执行天数
	uint32_t m_runDayCount;

protected:
	CConfig();

public:
	static CConfig& instance();
};

enum class TransType
{
	BEST_BUY,
	BEST_SELL,
	COUNT
};

enum class RangeTime
{
	RANGE0,
	RANGE10,
	RANGE20,
	RANGE30,
	RANGENEXT,
	COUNT
};

enum class ObserveTime
{
	TIME0930,
	TIME0940,
	TIME0950,
	TIME1000,
	TIME1010,
	TIME1020,
	TIME1030,
	TIME1040,
	TIME1050,
	TIME1100,
	TIME1110,
	TIME1120,
	TIME1310,
	TIME1320,
	TIME1330,
	TIME1340,
	TIME1350,
	TIME1400,
	TIME1410,
	TIME1420,
	TIME1430,
	TIME1440,
	TIME1450,
	COUNT
};

enum class Overall
{
	DATE,
	OPEN,
	CLOSE,
	COUNT
};

enum class StrategyMode
{
	S1100B1400,
	WAVE,
	COUNT
};