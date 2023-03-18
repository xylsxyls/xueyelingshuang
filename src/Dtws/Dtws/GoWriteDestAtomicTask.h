#pragma once
#include "ClickCommonAtomicTask.h"

class GoWriteDestAtomicTask : public ClickCommonAtomicTask
{
public:
	GoWriteDestAtomicTask();

public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	virtual bool CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	void setParam(const std::string& dest, int32_t clickIndex);

private:
	std::string m_dest;
	int32_t m_clickIndex;
};