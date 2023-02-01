#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class MoveTask : public CTask
{
public:
	MoveTask();

public:
	void DoTask();

	void setParam(const xyls::Point& pos, int32_t time = 0);

private:
	xyls::Point m_pos;
	int32_t m_time;
};