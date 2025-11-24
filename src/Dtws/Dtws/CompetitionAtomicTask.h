#pragma once
#include "ClickCommonAtomicTask.h"

class CompetitionAtomicTask : public ClickCommonAtomicTask
{
public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);
};