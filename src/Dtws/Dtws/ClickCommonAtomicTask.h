#pragma once
#include "ClickAtomicTask.h"

class ClickCommonAtomicTask : public ClickAtomicTask
{
public:
	void heroBlood(bool isStart, bool isFind);

public:
	void pic(bool isStart, bool isFind, const std::string& path, const xyls::Rect& rect, double sim);
};