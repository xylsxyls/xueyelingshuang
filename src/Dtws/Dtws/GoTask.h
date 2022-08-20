#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"
#include "SleepBaseTask.h"

class GoTask : public SleepBaseTask
{
public:
	GoTask();

public:
	void DoTask();

	void setParam(const xyls::Point& click, const xyls::Rect& placeRect, const std::string& placeName, int32_t clickIndex);

private:
	xyls::Point m_click;
	xyls::Rect m_placeRect;
	std::string m_placeName;
	int32_t m_clickIndex;
};