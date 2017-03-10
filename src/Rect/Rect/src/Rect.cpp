#include "Rect.h"

Rect::Rect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

void Rect::SetRect(int left, int top, int right, int bottom){
	this->left = left;
	this->top = top;
	this->right = right;
	this->bottom = bottom;
}

int Rect::GetWidth(){
	return right - left;
}

int Rect::GetHeight(){
	return bottom - top;
}