#pragma once
#include "RectMacro.h"
#include "Point/PointAPI.h"

class RectAPI Rect{
public:
	int left;
	int top;
	int right;
	int bottom;
	Rect();
	Rect(int left, int top, int right, int bottom);
	Rect(const Point& point);
	void SetRect(int left, int top, int right, int bottom);
	void SetRect(Point point);
	int GetWidth();
	int GetHeight();
	bool isPoint();
	bool isLine();
};