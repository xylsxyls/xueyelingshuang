#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"

class GoTask : public CTask
{
public:
	GoTask();

public:
	void DoTask();

	void setParam(const xyls::Point& clickTopPoint, const xyls::Rect& placeRect, const std::string& placeName, int32_t clickIndex);

private:
	xyls::Point m_clickTopPoint;
	xyls::Rect m_placeRect;
	std::string m_placeName;
	int32_t m_clickIndex;
};