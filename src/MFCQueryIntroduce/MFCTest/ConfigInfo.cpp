#include <SDKDDKVer.h>
#include "ConfigInfo.h"
#include "CGetPath/CGetPathAPI.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillSearchInt] = 10000;
    storage[ShowPersonInt] = 10001;
	storage[Split] = "~!@#$%^&*()";
	storage[txtPath] = CGetPath::GetCurrentExePath() + "save.data";
    storage[notMarriage] = "Î´»é";
    storage[marriaged] = "ÒÑ»é";
}