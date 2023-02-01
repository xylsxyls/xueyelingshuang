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
    void setPoint(int32_t x, int32_t y);
    int32_t x() const;
    int32_t y() const;
	bool empty() const;

    void setX(int32_t x);
    void setY(int32_t y);

	friend inline Point operator + (const Point& a, const Point& b)
	{
		return Point::add(a, b);
	}
	friend inline Point operator - (const Point& a, const Point& b)
	{
		return Point::sub(a, b);
	}
	friend inline bool operator == (const Point& a, const Point& b)
	{
		return Point::equal(a, b);
	}
	friend inline bool operator != (const Point& a, const Point& b)
	{
		return Point::notEqual(a, b);
	}

	static Point add(const Point& a, const Point& b);
	static Point sub(const Point& a, const Point& b);

	static bool equal(const Point& a, const Point& b);
	static bool notEqual(const Point& a, const Point& b);

private:
    int32_t m_x;
    int32_t m_y;
};

}