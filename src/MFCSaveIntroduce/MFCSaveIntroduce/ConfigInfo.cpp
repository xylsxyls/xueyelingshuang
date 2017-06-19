#include <SDKDDKVer.h>
#include "ConfigInfo.h"
#include "CGetPath/CGetPathAPI.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillPersonInt] = 10000;
    storage[ShowPersonInt] = 10001;
	storage[iniPath] = CGetPath::GetCurrentExePath() + "save.data";
	storage[iniPathBk] = CGetPath::GetCurrentExePath() + "savebk.data";
}