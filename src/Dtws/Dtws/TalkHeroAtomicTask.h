#pragma once
#include "ClickCommonAtomicTask.h"

class TalkHeroAtomicTask : public ClickCommonAtomicTask
{
public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);
};