#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class WaitTask : public CTask
{
public:
	void DoTask();

	void setParam(const xyls::Point& click);

private:
	xyls::Point m_click;
};