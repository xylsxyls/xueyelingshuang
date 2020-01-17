#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class OpenProcessTask : public CTask
{
public:
	OpenProcessTask();

public:
	void DoTask();

	void setParam(const std::string& path, int32_t findTime = 0, int32_t unfindTime = 0, const xyls::Point& point = xyls::Point(0, 0));

private:
	std::string m_path;
	xyls::Point m_point;
	int32_t m_findTime;
	int32_t m_unfindTime;
};