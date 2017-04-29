#pragma once
#include "CStorage/CStorageAPI.h"

class ConfigInfo{
public:
	enum{
		FillPersonInt,
        ShowPersonInt,
		Split,
		txtPath
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
public:
	ConfigInfo();
	CStorage storage;
};

extern ConfigInfo configInfo;