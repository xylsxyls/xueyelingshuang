#pragma once
#include "ClickCommonAtomicTask.h"

class SelectTeamerAtomicTask : public ClickCommonAtomicTask
{
public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	virtual bool CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);
};