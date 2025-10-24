#pragma once
#include <stdint.h>
#include <string>

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
	TIME1040,
	TIME1050,
	TIME1100,
	TIME1110,
	TIME1340,
	TIME1350,
	TIME1400,
	TIME1410,
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
	COUNT
};