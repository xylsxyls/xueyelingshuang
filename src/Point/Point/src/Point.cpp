#include "Point.h"

namespace xyls
{

Point::Point()
{
    m_x = 0;
    m_y = 0;
}

Point::Point(int32_t x, int32_t y)
{
    m_x = x;
    m_y = y;
}

#ifdef _MSC_VER
Point::Point(const POINT& point)
{
    m_x = point.x;
    m_y = point.y;
}
#endif

void Point::setPoint(int32_t x, int32_t y)
{
    m_x = x;
    m_y = y;
}

int32_t Point::x() const
{
    return m_x;
}

int32_t Point::y() const
{
    return m_y;
}

bool Point::empty() const
{
	return (m_x == 0) && (m_y == 0);
}

void Point::setX(int32_t x)
{
    m_x = x;
}

void Point::setY(int32_t y)
{
    m_y = y;
}

Point Point::add(const Point& a, const Point& b)
{
	return { a.m_x + b.m_x, a.m_y + b.m_y };
}

Point Point::sub(const Point& a, const Point& b)
{
	return{ a.m_x - b.m_x, a.m_y - b.m_y };
}

bool Point::equal(const Point& a, const Point& b)
{
	return (a.m_x == b.m_x) && (a.m_y == b.m_y);
}

bool Point::notEqual(const Point& a, const Point& b)
{
	return (a.m_x != b.m_x) || (a.m_y != b.m_y);
}

}