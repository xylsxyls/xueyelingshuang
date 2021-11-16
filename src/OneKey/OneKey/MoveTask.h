#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class MoveTask : public CTask
{
public:
	void DoTask();

	void setParam(const xyls::Point& pos);

private:
	xyls::Point m_pos;
};