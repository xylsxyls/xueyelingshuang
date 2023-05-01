#pragma once
#include "ClickCommonAtomicTask.h"

class SubmitAtomicTask : public ClickCommonAtomicTask
{
public:
	SubmitAtomicTask();

public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	void setParam(int32_t lineIndex);

private:
	int32_t m_lineIndex;
};