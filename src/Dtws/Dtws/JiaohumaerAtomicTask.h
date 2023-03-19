#pragma once
#include "ClickCommonAtomicTask.h"

class JiaohumaerAtomicTask : public ClickCommonAtomicTask
{
public:
	JiaohumaerAtomicTask();

public:
	virtual bool DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic);

	bool isFind();

private:
	bool m_isFind;
};