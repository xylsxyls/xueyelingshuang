#include "MulPoint.h"

MulPoint::MulPoint(){
	this->x = 0;
	this->y = 0;
}

MulPoint::MulPoint(int x, int y){
	this->x = x;
	this->y = y;
}

void MulPoint::SetMulPoint(int x, int y){
	this->x = x;
	this->y = y;
}