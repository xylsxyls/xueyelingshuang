#include "Rect.h"

Rect::Rect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

Rect::Rect(const Point& point){
	this->left = point.x;
	this->top = point.y;
	this->right = point.x;
	this->bottom = point.y;
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