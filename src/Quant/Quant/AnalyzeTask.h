#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class AnalyzeTask : public CTask
{
public:
	AnalyzeTask();

public:
	void DoTask();

	void StopTask();

	void setParam(const std::string& path);

private:
	std::string m_path;
	std::atomic<bool> m_exit;
};