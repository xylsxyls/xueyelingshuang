#pragma once
#include "PointMacro.h"
#include <stdint.h>

#ifdef _MSC_VER
#include <windows.h>
#endif

namespace xyls
{

class PointAPI Point
{
public:
	Point();
    Point(int32_t x, const int32_t y);

#ifdef _MSC_VER
    Point(const POINT& point);
#endif

public:
    void SetPoint(int32_t x, int32_t y);
    int32_t x() const;
    int32_t y() const;
	bool empty() const;

    void SetX(int32_t x);
    void SetY(int32_t y);

private:
    int32_t m_x;
    int32_t m_y;
};

}