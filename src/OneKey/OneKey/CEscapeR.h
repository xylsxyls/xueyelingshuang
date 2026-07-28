#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class CEscapeR : public CTask
{
public:
	CEscapeR();

public:
	void DoTask();

	void StopTask();

	void End(const xyls::Point& currentPos, bool rightHasDown);

private:
	std::atomic<bool> m_exit;
};