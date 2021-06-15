#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"

class AcceptTask : public CTask
{
public:
	AcceptTask();

public:
	void DoTask();

	void setParam(const xyls::Point& click, const xyls::Rect& findPicRect, int32_t times);

private:
	xyls::Point m_click;
	xyls::Rect m_findPicRect;
	std::string m_findPic;
	int32_t m_times;
};