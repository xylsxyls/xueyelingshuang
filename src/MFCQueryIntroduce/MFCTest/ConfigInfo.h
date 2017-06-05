#pragma once
#include "CStorage/CStorageAPI.h"

#define GetConfig(num, T) configInfo.storage[(num)].toValue<T>()

class ConfigInfo{
public:
	ConfigInfo();
	CStorage storage;
};

enum{
    FillSearchInt,
    ShowPersonInt,
    Split,
    txtPath,
    notMarriage,
	notMarriage1,
    hasMarriage,
	hasMarriage1,
	hasMarriage2,
	noHouse,
	noHouse1,
    hasHouse,
	hasHouse1,
	hasHouse2,
	junior,
	junior1,
	junior2,
	college,
	college1,
	college2,
	college3,
	college4,
	college5,
	college6,
	graduate,
	graduate1,
	graduate2,
	graduate3,
	graduate4,
	philosophy,
	philosophy1,
	philosophy2,
    notHasHouse,
	noMatter,
	maxPeople
};
enum{
    txtPerson = 0,
    txtOffset = 1,
    firstPerson = 0,
    name = 0,
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