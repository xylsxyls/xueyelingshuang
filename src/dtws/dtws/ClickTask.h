#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include "Point/PointAPI.h"
#include <vector>
#include <stdint.h>
#include "BaseTask.h"

class ClickTask : public BaseTask
{
public:
	ClickTask();

public:
	void DoTask();

	void setParam(uint32_t checkWorkThreadId, const std::vector<std::pair<xyls::Point, int32_t>>& vecClick);

private:
	uint32_t m_checkWorkThreadId;
	std::vector<std::pair<xyls::Point, int32_t>> m_vecClick;
};