#include <SDKDDKVer.h>
#include "ConfigInfo.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillPersonInt] = 10000;
    storage[ShowPersonInt] = 10001;
	storage[Split] = string("~!@#$%^&*()");
	storage[txtPath] = string("save.data");
}