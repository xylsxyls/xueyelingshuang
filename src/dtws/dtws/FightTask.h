#pragma once
#include "CheckTask.h"
#include <string>
#include "Point/PointAPI.h"
#include "BaseTask.h"

class FightTask : public CheckTask
{
public:
	FightTask();

public:
	void DoTask();

	void setParam(const xyls::Point& dest, int32_t moveTime, int32_t skillNum, int32_t fightNum, const xyls::Point& over);

private:
	xyls::Point m_dest;
	xyls::Point m_over;
	int32_t m_moveTime;
	int32_t m_fightNum;
	int32_t m_skillNum;
};