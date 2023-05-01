#pragma once
#include "ClickCommonAtomicTask.h"

class ClickWindowAtomicTask : public ClickCommonAtomicTask
{
public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	virtual bool CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	void setParam(const xyls::Point& oppositePoint);

private:
	xyls::Point m_oppositePoint;
};