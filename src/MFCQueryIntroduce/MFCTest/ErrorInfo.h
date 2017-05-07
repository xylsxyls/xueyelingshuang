#pragma once
#include "CStorage/CStorageAPI.h"

class ErrorInfo{
public:
	enum{
		Success = 0,
		Name = 1,
		Birth,
		Sex,
		Marriage,
		Education
	};
public:
	ErrorInfo();
	CStorage storage;
};

extern ErrorInfo errorInfo;