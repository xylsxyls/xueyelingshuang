#pragma once
#include "CstrValue.h"

class CszOneValue{
public:
	int position;
	CstrValue strValue;

public:
	CszOneValue();
	CszOneValue(int position,CstrValue strValue);
	~CszOneValue();
};