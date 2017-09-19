#include "Rect.h"

namespace xyls {

Rect::Rect(){
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

Rect::Rect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

Rect::Rect(const Point& origin, int width, int height)
{
	this->left = origin.x;
	this->top = origin.y;
	this->right = origin.x + width;
	this->bottom = origin.y + height;
}

#ifdef _WIN32
Rect::Rect(const RECT& rect){
    left = rect.left;
    top = rect.top;
    right = rect.right;
    bottom = rect.bottom;
}
#endif

Rect::Rect(const Point& point){
	left = point.x;
	top = point.y;
	right = point.x;
	bottom = point.y;
}

void Rect::SetRect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

void Rect::SetRect(const Point& point){
	this->left = point.x;
	this->top = point.y;
	this->right = point.x;
	this->bottom = point.y;
}

int Rect::GetWidth()const{
	return right - left;
}

int Rect::GetHeight()const{
	return bottom - top;
}

bool Rect::isPoint()const{
	return (left == right) && (top == bottom);
}

bool Rect::isLine()const{
	return ((left == right) + (top == bottom)) == 1;
}

bool Rect::isHorizontalLine()const
{
    return (top == bottom) && (left != right);
}

bool Rect::isVerticalLine()const
{
    return (left == right) && (top != bottom);
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
			if (left == right)
			{
				int leftright = (bkRect.left + bkRect.right) / 2;
				return Rect(leftright, bkRect.top, leftright, bkRect.bottom);
			}
			else
			{
				int topbottom = (bkRect.top + bkRect.bottom) / 2;
				return Rect(bkRect.left, topbottom, bkRect.right, topbottom);
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
				return Rect(bkRect.left, bkRect.top + heightChange, bkRect.right, bkRect.bottom - heightChange);
			}
			else
			{
                int width = (int)(GetWidth() * (double)bkRect.GetHeight() / GetHeight());
                int widthChange = (int)((bkRect.GetWidth() - width) / 2.0);
				return Rect(bkRect.left + widthChange, bkRect.top, bkRect.right - widthChange, bkRect.bottom);
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
				int leftright = bkRect.left;
                int topbottom = (int)((bkRect.top + bkRect.bottom) / 2.0);
				return Rect(leftright, topbottom, leftright, topbottom);
			}
			else
			{
				int topbottom = bkRect.top;
                int leftright = (int)((bkRect.left + bkRect.right) / 2.0);
				return Rect(leftright, topbottom, leftright, topbottom);
			}
		}
		else
		{
            int leftright = (int)((bkRect.left + bkRect.right) / 2.0);
            int topbottom = (int)((bkRect.top + bkRect.bottom) / 2.0);
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
                        return Rect(bkRect.left, bkRect.top + offset, bkRect.right, bkRect.bottom - offset);
                    }
                    else
                    {
                        return bkRect;
                    }
                }
                else
                {
                    return Rect(Point((int)((bkRect.left + bkRect.right) / 2.0), bkRect.top));
                }
            }
            else
            {
                if (bkRect.isVerticalLine())
                {
                    return Rect(Point(bkRect.left, (int)((bkRect.top + bkRect.bottom) / 2.0)));
                }
                else
                {
                    if (GetWidth() < bkRect.GetWidth())
                    {
                        int offset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                        return Rect(bkRect.left + offset, bkRect.top, bkRect.right - offset, bkRect.bottom);
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
                    int leftright = (bkRect.left + bkRect.right) / 2;
                    int offset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                    return Rect(leftright, bkRect.top + offset, leftright, bkRect.bottom - offset);
                }
                else
                {
                    int leftright = (bkRect.left + bkRect.right) / 2;
                    return Rect(leftright, bkRect.top, leftright, bkRect.bottom);
                }
			}
			else
			{
                if (GetWidth() < bkRect.GetWidth())
                {
                    int topbottom = (bkRect.top + bkRect.bottom) / 2;
                    int offset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                    return Rect(bkRect.left + offset, topbottom, bkRect.right - offset, topbottom);
                }
                else
                {
                    int topbottom = (bkRect.top + bkRect.bottom) / 2;
                    return Rect(bkRect.left, topbottom, bkRect.right, topbottom);
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
                    return Rect(bkRect.left, bkRect.top + offset, bkRect.right, bkRect.bottom - offset);
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
                    return Rect(bkRect.left + offset, bkRect.top, bkRect.right - offset, bkRect.bottom);
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
                    return Rect(bkRect.left, bkRect.top + heightChange, bkRect.right, bkRect.bottom - heightChange);
                }
                else
                {
                    int width = (int)(GetWidth() * (double)bkRect.GetHeight() / GetHeight());
                    int widthChange = (int)((bkRect.GetWidth() - width) / 2.0);
                    return Rect(bkRect.left + widthChange, bkRect.top, bkRect.right - widthChange, bkRect.bottom);
                }
            }
            else
            {
                int widthoffset = (int)((bkRect.GetWidth() - GetWidth()) / 2.0);
                int heightoffset = (int)((bkRect.GetHeight() - GetHeight()) / 2.0);
                return Rect(bkRect.left + widthoffset, bkRect.top + heightoffset, bkRect.right - widthoffset, bkRect.bottom - heightoffset);
            }
		}
	}
}

Rect Rect::Over(const Rect& otherRect)
{
    RECT rect;
    ::IntersectRect(&rect, &(RECT(*this)), &(RECT(otherRect)));
    return Rect(rect);
}

double Rect::Gettan()const
{
	return GetHeight() / (double)GetWidth();
}

#ifdef _WIN32
Rect::operator RECT()const{
    RECT rect;
    rect.left = left;
    rect.right = right;
    rect.top = top;
    rect.bottom = bottom;
    return rect;
}
#endif

}