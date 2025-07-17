#pragma once
#include <stdint.h>
#include <string>

#define g_config CConfig::instance()

struct CConfig
{
	int32_t m_screenPixel = 1920 * 1080;
	std::string m_currentExePath;
	std::string m_computerName;

protected:
	CConfig();

public:
	static CConfig& instance();
};