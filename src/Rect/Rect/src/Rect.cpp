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

int Rect::GetWidth(){
	return right - left;
}

int Rect::GetHeight(){
	return bottom - top;
}

bool Rect::isPoint(){
	return (left == right) && (top == bottom);
}

bool Rect::isLine(){
	return ((left == right) + (top == bottom)) == 1;
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