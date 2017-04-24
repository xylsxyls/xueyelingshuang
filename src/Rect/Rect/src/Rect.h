#pragma once
#include "RectMacro.h"
#include "Point/PointAPI.h"

#ifdef _WIN32
#include <windows.h>
#endif

class RectAPI Rect{
public:
	int left;
	int top;
	int right;
	int bottom;
	Rect();

#ifdef _WIN32
    Rect(const RECT& rect);
#endif

	Rect(int left, int top, int right, int bottom);
	Rect(const Point& point);
	void SetRect(int left, int top, int right, int bottom);
	void SetRect(Point point);
	int GetWidth();
	int GetHeight();
	bool isPoint();
	bool isLine();

#ifdef _WIN32
    operator RECT();
#endif
};