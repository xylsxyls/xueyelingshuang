#pragma once
#include "CStorage/CStorageAPI.h"

class ConfigInfo{
public:
	enum{
		FillSearchInt,
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
	enum{
		txtPerson,
		txtOffset
	};
	enum{
		notMarriage,
		marriaged,
		hasHouse,
		notHasHouse
	};
public:
	ConfigInfo();
	CStorage storage;
};

extern ConfigInfo configInfo;