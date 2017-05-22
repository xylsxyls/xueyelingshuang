#include <SDKDDKVer.h>
#include "ConfigInfo.h"
#include "CGetPath/CGetPathAPI.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillPersonInt] = 10000;
    storage[ShowPersonInt] = 10001;
	storage[txtPath] = CGetPath::GetCurrentExePath() + "save.data";
    storage[txtPathBk] = CGetPath::GetCurrentExePath() + "savebk.data";
}