#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Rect/RectAPI.h"

class AcceptTask : public CTask
{
public:
	AcceptTask();

public:
	void DoTask();

	void setParam(const xyls::Point& click, const xyls::Rect& findPicRect, const std::vector<xyls::Point>& acceptPoint);

private:
	xyls::Point m_click;
	xyls::Rect m_findPicRect;
	std::string m_findPic;
	std::vector<xyls::Point> m_acceptPoint;
};