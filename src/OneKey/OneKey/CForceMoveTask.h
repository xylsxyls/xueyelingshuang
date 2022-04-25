#pragma once
#include "CSleepTask.h"
#include "Point/PointAPI.h"

class CForceMoveTask : public CSleepTask
{
public:
	void disableMouse();

	void enableMouse();

	void moveAbsolute(const xyls::Point& point, int32_t sleepTime);
};