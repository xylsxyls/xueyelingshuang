#pragma once
#include "ClickCommonAtomicTask.h"

class ConfirmAtomicTask : public ClickCommonAtomicTask
{
public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	void setConfirmParam(const std::vector<xyls::Point>& vecConfirm);

private:
	std::vector<xyls::Point> m_vecConfirm;
};