#pragma once
#include "PointMacro.h"

namespace xyls {

class PointAPI Point{
public:
	int x;
	int y;
	Point();
	Point(int x, int y);
	void SetPoint(int x, int y);
};

}