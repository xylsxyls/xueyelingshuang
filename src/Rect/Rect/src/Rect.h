#pragma once
#include "RectMacro.h"

class RectAPI Rect{
public:
	int left;
	int top;
	int right;
	int bottom;
	Rect(int left, int top, int right, int bottom);
	void SetRect(int left, int top, int right, int bottom);
	int GetWidth();
	int GetHeight();
};