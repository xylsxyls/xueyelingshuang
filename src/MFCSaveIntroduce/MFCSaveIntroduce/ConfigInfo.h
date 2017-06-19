#pragma once
#include "CStorage/CStorageAPI.h"

#define GetConfig(num, T) configInfo.storage[(num)].toValue<T>()

class ConfigInfo{
public:
	ConfigInfo();
	CStorage storage;
};

enum{
    FillPersonInt,
    ShowPersonInt,
    Split,
    iniPath,
	iniPathBk
};
enum{
    name,
    birth,
    sex,
    marriage,
    education,
    tall,
    weight,
    house,
    car,
    household,
    houseAttri,
    houseAddr,
    jobName,
    jobNature,
    salary,
    mobile,
    qq,
    weChat,
    fatherJob,
    fatherPension,
    motherJob,
    motherPension,
    picture1,
    picture2,
    picture3,
    introduce
};

extern ConfigInfo configInfo;