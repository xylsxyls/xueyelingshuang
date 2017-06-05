#include "Rect.h"

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

void Rect::SetRect(Point point){
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
				int height = GetHeight() * (double)bkRect.GetWidth() / GetWidth();
				int heightChange = (bkRect.GetHeight() - height) / 2.0;
				return Rect(bkRect.left, bkRect.top + heightChange, bkRect.right, bkRect.bottom - heightChange);
			}
			else
			{
				int width = GetWidth() * (double)bkRect.GetHeight() / GetHeight();
				int widthChange = (bkRect.GetWidth() - width) / 2.0;
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
			if (bkRect.left == bkRect.right)
			{
				int leftright = bkRect.left;
				int topbottom = (bkRect.top + bkRect.bottom) / 2.0;
				return Rect(leftright, topbottom, leftright, topbottom);
			}
			else
			{
				int topbottom = bkRect.top;
				int leftright = (bkRect.left + bkRect.right) / 2.0;
				return Rect(leftright, topbottom, leftright, topbottom);
			}
		}
		else
		{
			int leftright = (bkRect.left + bkRect.right) / 2.0;
			int topbottom = (bkRect.top + bkRect.bottom) / 2.0;
			return Rect(leftright, topbottom, leftright, topbottom);
		}
	}
	///////////
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
				int height = GetHeight() * (double)bkRect.GetWidth() / GetWidth();
				int heightChange = (bkRect.GetHeight() - height) / 2.0;
				return Rect(bkRect.left, bkRect.top + heightChange, bkRect.right, bkRect.bottom - heightChange);
			}
			else
			{
				int width = GetWidth() * (double)bkRect.GetHeight() / GetHeight();
				int widthChange = (bkRect.GetWidth() - width) / 2.0;
				return Rect(bkRect.left + widthChange, bkRect.top, bkRect.right - widthChange, bkRect.bottom);
			}
		}
	}
}

Rect Rect::Over(const Rect& bkRect)
{

}

double Rect::Gettan()const
{
	return GetHeight() / (double)GetWidth();
}

#ifdef _WIN32
Rect::operator RECT(){
    RECT rect;
    rect.left = left;
    rect.right = right;
    rect.top = top;
    rect.bottom = bottom;
    return rect;
}
#endif