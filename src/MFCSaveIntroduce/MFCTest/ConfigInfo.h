#pragma once
#include "CStorage/CStorageAPI.h"

class ConfigInfo{
public:
	enum{
		FillPersonInt,
		Split,
		txtPath
	};
public:
	ConfigInfo();
	CStorage storage;
};

extern ConfigInfo configInfo;