#pragma once
#include "ClickAtomicTask.h"

class ClickCommonAtomicTask : public ClickAtomicTask
{
public:
	void heroBlood(bool isStart, bool isFind);

	void go(bool isStart, bool isFind);

	void accept(bool isStart, bool isFind);

	void jiaohumaer(bool isStart, bool isFind);

	void lowSleep(int32_t sleepTime);

public:
	void pic(bool isStart, bool isFind, const std::string& path, const xyls::Rect& rect, double sim);
};