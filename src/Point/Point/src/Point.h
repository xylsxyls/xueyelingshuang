#pragma once
#include "PointMacro.h"
#include <stdint.h>
#include <windows.h>

namespace xyls
{

class PointAPI Point
{
public:
	Point();
    Point(int32_t x, const int32_t y);
    Point(const POINT& point);
    void SetPoint(int32_t x, int32_t y);
    int32_t x() const;
    int32_t y() const;

    void SetX(int32_t x);
    void SetY(int32_t y);

private:
    int32_t m_x;
    int32_t m_y;
};

}