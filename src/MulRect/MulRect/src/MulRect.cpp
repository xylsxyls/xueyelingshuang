#include "MulRect.h"

MulRect::MulRect(){
	left = 0;
	top = 0;
	right = 0;
	bottom = 0;
}

MulRect::MulRect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

#ifdef _WIN32
MulRect::MulRect(const RECT& rect){
    left = rect.left;
    top = rect.top;
    right = rect.right;
    bottom = rect.bottom;
}
#endif

MulRect::MulRect(const Point& point){
	left = point.x;
	top = point.y;
	right = point.x;
	bottom = point.y;
}

void MulRect::SetMulRect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

void MulRect::SetMulRect(Point point){
	this->left = point.x;
	this->top = point.y;
	this->right = point.x;
	this->bottom = point.y;
}

int MulRect::GetWidth()const{
	return right - left;
}

int MulRect::GetHeight()const{
	return bottom - top;
}

bool MulRect::isPoint()const{
	return (left == right) && (top == bottom);
}

bool MulRect::isLine()const{
	return ((left == right) + (top == bottom)) == 1;
}

bool MulRect::isHorizontalLine()const
{
    return (top == bottom) && (left != right);
}

bool MulRect::isVerticalLine()const
{
    return (left == right) && (top != bottom);
}

MulRect MulRect::Adapt(const MulRect& bkMulRect)
{
	if (isPoint() == true)
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
			return MulRect();
		}
		else
		{
			return MulRect();
		}
	}
	else if (isLine() == true)
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
			return bkMulRect;
		}
		else
		{
			if (left == right)
			{
				int leftright = (bkMulRect.left + bkMulRect.right) / 2;
				return MulRect(leftright, bkMulRect.top, leftright, bkMulRect.bottom);
			}
			else
			{
				int topbottom = (bkMulRect.top + bkMulRect.bottom) / 2;
				return MulRect(bkMulRect.left, topbottom, bkMulRect.right, topbottom);
			}
		}
	}
	else
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
			return bkMulRect;
		}
		else
		{
			if (Gettan() < bkMulRect.Gettan())
			{
				int height = (int)(GetHeight() * (double)bkMulRect.GetWidth() / GetWidth());
                int heightChange = (int)((bkMulRect.GetHeight() - height) / 2.0);
				return MulRect(bkMulRect.left, bkMulRect.top + heightChange, bkMulRect.right, bkMulRect.bottom - heightChange);
			}
			else
			{
                int width = (int)(GetWidth() * (double)bkMulRect.GetHeight() / GetHeight());
                int widthChange = (int)((bkMulRect.GetWidth() - width) / 2.0);
				return MulRect(bkMulRect.left + widthChange, bkMulRect.top, bkMulRect.right - widthChange, bkMulRect.bottom);
			}
		}
	}
}

MulRect MulRect::SetIn(const MulRect& bkMulRect)
{
	if (isPoint() == true)
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
            if (bkMulRect.isVerticalLine())
			{
				int leftright = bkMulRect.left;
                int topbottom = (int)((bkMulRect.top + bkMulRect.bottom) / 2.0);
				return MulRect(leftright, topbottom, leftright, topbottom);
			}
			else
			{
				int topbottom = bkMulRect.top;
                int leftright = (int)((bkMulRect.left + bkMulRect.right) / 2.0);
				return MulRect(leftright, topbottom, leftright, topbottom);
			}
		}
		else
		{
            int leftright = (int)((bkMulRect.left + bkMulRect.right) / 2.0);
            int topbottom = (int)((bkMulRect.top + bkMulRect.bottom) / 2.0);
			return MulRect(leftright, topbottom, leftright, topbottom);
		}
	}
	else if (isLine() == true)
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
            if (isVerticalLine())
            {
                if (bkMulRect.isVerticalLine())
                {
                    if (GetHeight() < bkMulRect.GetHeight())
                    {
                        int offset = (int)((bkMulRect.GetHeight() - GetHeight()) / 2.0);
                        return MulRect(bkMulRect.left, bkMulRect.top + offset, bkMulRect.right, bkMulRect.bottom - offset);
                    }
                    else
                    {
                        return bkMulRect;
                    }
                }
                else
                {
                    return MulRect(Point((int)((bkMulRect.left + bkMulRect.right) / 2.0), bkMulRect.top));
                }
            }
            else
            {
                if (bkMulRect.isVerticalLine())
                {
                    return MulRect(Point(bkMulRect.left, (int)((bkMulRect.top + bkMulRect.bottom) / 2.0)));
                }
                else
                {
                    if (GetWidth() < bkMulRect.GetWidth())
                    {
                        int offset = (int)((bkMulRect.GetWidth() - GetWidth()) / 2.0);
                        return MulRect(bkMulRect.left + offset, bkMulRect.top, bkMulRect.right - offset, bkMulRect.bottom);
                    }
                    else
                    {
                        return bkMulRect;
                    }
                }
            }
		}
		else
		{
			if (isVerticalLine())
			{
                if (GetHeight() < bkMulRect.GetHeight())
                {
                    int leftright = (bkMulRect.left + bkMulRect.right) / 2;
                    int offset = (int)((bkMulRect.GetHeight() - GetHeight()) / 2.0);
                    return MulRect(leftright, bkMulRect.top + offset, leftright, bkMulRect.bottom - offset);
                }
                else
                {
                    int leftright = (bkMulRect.left + bkMulRect.right) / 2;
                    return MulRect(leftright, bkMulRect.top, leftright, bkMulRect.bottom);
                }
			}
			else
			{
                if (GetWidth() < bkMulRect.GetWidth())
                {
                    int topbottom = (bkMulRect.top + bkMulRect.bottom) / 2;
                    int offset = (int)((bkMulRect.GetWidth() - GetWidth()) / 2.0);
                    return MulRect(bkMulRect.left + offset, topbottom, bkMulRect.right - offset, topbottom);
                }
                else
                {
                    int topbottom = (bkMulRect.top + bkMulRect.bottom) / 2;
                    return MulRect(bkMulRect.left, topbottom, bkMulRect.right, topbottom);
                }
			}
		}
	}
	else
	{
		if (bkMulRect.isPoint() == true)
		{
			return bkMulRect;
		}
		else if (bkMulRect.isLine() == true)
		{
            if (bkMulRect.isVerticalLine())
            {
                if (GetHeight() < bkMulRect.GetHeight())
                {
                    int offset = (int)((bkMulRect.GetHeight() - GetHeight()) / 2.0);
                    return MulRect(bkMulRect.left, bkMulRect.top + offset, bkMulRect.right, bkMulRect.bottom - offset);
                }
                else
                {
                    return bkMulRect;
                }
            }
            else
            {
                if (GetWidth() < bkMulRect.GetWidth())
                {
                    int offset = (int)((bkMulRect.GetWidth() - GetWidth()) / 2.0);
                    return MulRect(bkMulRect.left + offset, bkMulRect.top, bkMulRect.right - offset, bkMulRect.bottom);
                }
                else
                {
                    return bkMulRect;
                }
            }
		}
        else
        {
            if (GetWidth() >= bkMulRect.GetWidth() || GetHeight() >= bkMulRect.GetHeight())
            {
                if (Gettan() < bkMulRect.Gettan())
                {
                    int height = (int)(GetHeight() * (double)bkMulRect.GetWidth() / GetWidth());
                    int heightChange = (int)((bkMulRect.GetHeight() - height) / 2.0);
                    return MulRect(bkMulRect.left, bkMulRect.top + heightChange, bkMulRect.right, bkMulRect.bottom - heightChange);
                }
                else
                {
                    int width = (int)(GetWidth() * (double)bkMulRect.GetHeight() / GetHeight());
                    int widthChange = (int)((bkMulRect.GetWidth() - width) / 2.0);
                    return MulRect(bkMulRect.left + widthChange, bkMulRect.top, bkMulRect.right - widthChange, bkMulRect.bottom);
                }
            }
            else
            {
                int widthoffset = (int)((bkMulRect.GetWidth() - GetWidth()) / 2.0);
                int heightoffset = (int)((bkMulRect.GetHeight() - GetHeight()) / 2.0);
                return MulRect(bkMulRect.left + widthoffset, bkMulRect.top + heightoffset, bkMulRect.right - widthoffset, bkMulRect.bottom - heightoffset);
            }
		}
	}
}

MulRect MulRect::Over(const MulRect& otherMulRect)
{
    RECT rect;
    ::IntersectRect(&rect, &(RECT(*this)), &(RECT(otherMulRect)));
    return MulRect(rect);
}

double MulRect::Gettan()const
{
	return GetHeight() / (double)GetWidth();
}

#ifdef _WIN32
MulRect::operator RECT()const{
    RECT rect;
    rect.left = left;
    rect.right = right;
    rect.top = top;
    rect.bottom = bottom;
    return rect;
}
#endif