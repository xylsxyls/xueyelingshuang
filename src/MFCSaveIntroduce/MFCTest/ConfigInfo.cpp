#include "ConfigInfo.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillPersonInt] = 10000;
	storage[Split] = string("~!@#$%^&*()");
	storage[txtPath] = string("save.data");
}