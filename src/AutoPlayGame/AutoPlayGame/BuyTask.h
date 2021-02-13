#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class BuyTask : public CTask
{
public:
	void DoTask();

	void setParam(const xyls::Point buy1, const xyls::Point buy2, const xyls::Point buy3);

private:
	xyls::Point m_buy1;
	xyls::Point m_buy2;
	xyls::Point m_buy3;
};