#include "Rect.h"

namespace xyls
{

Rect::Rect()
{
	m_left = 0;
	m_top = 0;
	m_right = 0;
	m_bottom = 0;
}

Rect::Rect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
}

Rect::Rect(const Point& origin, int32_t width, int32_t height)
{
	m_left = origin.x();
	m_top = origin.y();
    m_right = origin.x() + width;
    m_bottom = origin.y() + height;
}

#ifdef _MSC_VER
Rect::Rect(const RECT& rect)
{
    m_left = rect.left;
    m_top = rect.top;
    m_right = rect.right;
    m_bottom = rect.bottom;
}
#endif

Rect::Rect(const Point& point)
{
    m_left = point.x();
    m_top = point.y();
    m_right = point.x();
    m_bottom = point.y();
}

Rect::Rect(const Point& point, int32_t margin)
{
    m_left = point.x() - margin;
    m_top = point.y() - margin;
    m_right = point.x() + margin;
    m_bottom = point.y() + margin;
}

void Rect::SetRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
}

void Rect::SetRect(const Point& point)
{
	m_left = point.x();
    m_top = point.y();
    m_right = point.x();
    m_bottom = point.y();
}

int32_t Rect::GetWidth() const
{
	return m_right - m_left;
}

int32_t Rect::GetHeight() const
{
	return m_bottom - m_top;
}

bool Rect::isPoint() const
{
	return (m_left == m_right) && (m_top == m_bottom);
}

bool Rect::isLine() const
{
	return ((m_left == m_right) + (m_top == m_bottom)) == 1;
}

bool Rect::empty() const
{
	return (m_left == 0) && (m_top == 0) && (m_right == 0) && (m_bottom == 0);
}

int32_t Rect::Left() const
{
    return m_left;
}

int32_t Rect::Top() const
{
    return m_top;
}

int32_t Rect::Right() const
{
    return m_right;
}

int32_t Rect::Bottom() const
{
    return m_bottom;
}

void Rect::SetLeft(int32_t left)
{
    m_left = left;
}

void Rect::SetTop(int32_t top)
{
    m_top = top;
}

void Rect::SetRight(int32_t right)
{
    m_right = right;
}

void Rect::SetBottom(int32_t bottom)
{
    m_bottom = bottom;
}

bool Rect::contain(const xyls::Point& point)
{
	return point.x() >= m_left && point.y() >= m_top && point.x() <= m_right && point.y() <= m_bottom;
}

bool Rect::operator==(const Rect& rect) const
{
	return m_left == rect.m_left && m_top == rect.m_top && m_right == rect.m_right && m_bottom == rect.m_bottom;
}

bool Rect::isHorizontalLine()const
{
    return (m_top == m_bottom) && (m_left != m_right);
}

bool Rect::isVerticalLine()const
{
    return (m_left == m_right) && (m_top != m_bottom);
}

Rect Rect::Adapt(const Rect& bkRect)
{
	if (isPoint() == true)
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
			return Rect();
		}
		else
		{
			return Rect();
		}
	}
	else if (isLine() == true)
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
			return bkRect;
		}
		else
		{
			if (m_left == m_right)
			{
				int leftright = (bkRect.m_left + bkRect.m_right) / 2;
				return Rect(leftright, bkRect.m_top, leftright, bkRect.m_bottom);
			}
			else
			{
				int topbottom = (bkRect.m_top + bkRect.m_bottom) / 2;
				return Rect(bkRect.m_left, topbottom, bkRect.m_right, topbottom);
			}
		}
	}
	else
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
			return bkRect;
		}
		else
		{
			if (Gettan() < bkRect.Gettan())
			{
				int height = (int)(GetHeight() * (double)bkRect.GetWidth() / GetWidth());
                int heightChange = (int)((bkRect.GetHeight() - height) / 2.0);
				return Rect(bkRect.m_left, bkRect.m_top + heightChange, bkRect.m_right, bkRect.m_bottom - heightChange);
			}
			else
			{
                int width = (int)(GetWidth() * (double)bkRect.GetHeight() / GetHeight());
                int widthChange = (int)((bkRect.GetWidth() - width) / 2.0);
				return Rect(bkRect.m_left + widthChange, bkRect.m_top, bkRect.m_right - widthChange, bkRect.m_bottom);
			}
		}
	}
}

Rect Rect::SetIn(const Rect& bkRect)
{
	if (isPoint() == true)
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
            if (bkRect.isVerticalLine())
			{
				int leftright = bkRect.m_left;
                int topbottom = (int)((bkRect.m_top + bkRect.m_bottom) / 2.0);
				return Rect(leftright, topbottom, leftright, topbottom);
			}
			else
			{
				int topbottom = bkRect.m_top;
                int leftright = (int)((bkRect.m_left + bkRect.m_right) / 2.0);
				return Rect(leftright, topbottom, leftright, topbottom);
			}
		}
		else
		{
            int leftright = (int)((bkRect.m_left + bkRect.m_right) / 2.0);
            int topbottom = (int)((bkRect.m_top + bkRect.m_bottom) / 2.0);
			return Rect(leftright, topbottom, leftright, topbottom);
		}
	}
	else if (isLine() == true)
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
            if (isVerticalLine())
            {
                if (bkRect.isVerticalLine())
                {
                    if (GetHeight() < bkRect.GetHeight())
                    {
                        int offset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                        return Rect(bkRect.m_left, bkRect.m_top + offset, bkRect.m_right, bkRect.m_bottom - offset);
                    }
                    else
                    {
                        return bkRect;
                    }
                }
                else
                {
                    return Rect(Point((int)((bkRect.m_left + bkRect.m_right) / 2.0), bkRect.m_top));
                }
            }
            else
            {
                if (bkRect.isVerticalLine())
                {
                    return Rect(Point(bkRect.m_left, (int)((bkRect.m_top + bkRect.m_bottom) / 2.0)));
                }
                else
                {
                    if (GetWidth() < bkRect.GetWidth())
                    {
                        int offset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                        return Rect(bkRect.m_left + offset, bkRect.m_top, bkRect.m_right - offset, bkRect.m_bottom);
                    }
                    else
                    {
                        return bkRect;
                    }
                }
            }
		}
		else
		{
			if (isVerticalLine())
			{
                if (GetHeight() < bkRect.GetHeight())
                {
                    int leftright = (bkRect.m_left + bkRect.m_right) / 2;
                    int offset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                    return Rect(leftright, bkRect.m_top + offset, leftright, bkRect.m_bottom - offset);
                }
                else
                {
                    int leftright = (bkRect.m_left + bkRect.m_right) / 2;
                    return Rect(leftright, bkRect.m_top, leftright, bkRect.m_bottom);
                }
			}
			else
			{
                if (GetWidth() < bkRect.GetWidth())
                {
                    int topbottom = (bkRect.m_top + bkRect.m_bottom) / 2;
                    int offset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                    return Rect(bkRect.m_left + offset, topbottom, bkRect.m_right - offset, topbottom);
                }
                else
                {
                    int topbottom = (bkRect.m_top + bkRect.m_bottom) / 2;
                    return Rect(bkRect.m_left, topbottom, bkRect.m_right, topbottom);
                }
			}
		}
	}
	else
	{
		if (bkRect.isPoint() == true)
		{
			return bkRect;
		}
		else if (bkRect.isLine() == true)
		{
            if (bkRect.isVerticalLine())
            {
                if (GetHeight() < bkRect.GetHeight())
                {
                    int offset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                    return Rect(bkRect.m_left, bkRect.m_top + offset, bkRect.m_right, bkRect.m_bottom - offset);
                }
                else
                {
                    return bkRect;
                }
            }
            else
            {
                if (GetWidth() < bkRect.GetWidth())
                {
                    int offset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                    return Rect(bkRect.m_left + offset, bkRect.m_top, bkRect.m_right - offset, bkRect.m_bottom);
                }
                else
                {
                    return bkRect;
                }
            }
		}
        else
        {
            if (GetWidth() >= bkRect.GetWidth() || GetHeight() >= bkRect.GetHeight())
            {
                if (Gettan() < bkRect.Gettan())
                {
                    int height = (int)(GetHeight() * (double)bkRect.GetWidth() / GetWidth());
                    int heightChange = (int)((bkRect.GetHeight() - height) / 2.0);
                    return Rect(bkRect.m_left, bkRect.m_top + heightChange, bkRect.m_right, bkRect.m_bottom - heightChange);
                }
                else
                {
                    int width = (int)(GetWidth() * (double)bkRect.GetHeight() / GetHeight());
                    int widthChange = (int)((bkRect.GetWidth() - width) / 2.0);
                    return Rect(bkRect.m_left + widthChange, bkRect.m_top, bkRect.m_right - widthChange, bkRect.m_bottom);
                }
            }
            else
            {
                int widthoffset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                int heightoffset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                return Rect(bkRect.m_left + widthoffset, bkRect.m_top + heightoffset, bkRect.m_right - widthoffset, bkRect.m_bottom - heightoffset);
            }
		}
	}
}

#ifdef _MSC_VER
Rect Rect::Over(const Rect& otherRect)
{
    RECT rect;
    ::IntersectRect(&rect, &(RECT(*this)), &(RECT(otherRect)));
    return Rect(rect);
}
#endif

double Rect::Gettan() const
{
	return GetHeight() / (double)GetWidth();
}

#ifdef _MSC_VER
Rect::operator RECT() const
{
    RECT rect;
    rect.left = m_left;
    rect.right = m_right;
    rect.top = m_top;
    rect.bottom = m_bottom;
    return rect;
}
#endif

}