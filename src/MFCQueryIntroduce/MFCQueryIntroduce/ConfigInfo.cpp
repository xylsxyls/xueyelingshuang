#include <SDKDDKVer.h>
#include "stdafx.h"
#include "ConfigInfo.h"
#include "CGetPath/CGetPathAPI.h"

ConfigInfo configInfo;

ConfigInfo::ConfigInfo(){
	storage[FillSearchInt] = 10000;
    storage[ShowPersonInt] = 10001;
	storage[Split] = "~!@#$%^&*()";
	storage[txtPath] = CGetPath::GetCurrentExePath() + "save.data";
    storage[notMarriage] = "未婚";
	storage[notMarriage1] = "未婚";
    storage[hasMarriage] = "已婚";
	storage[hasMarriage1] = "短婚未育";
	storage[hasMarriage2] = "已婚已育";
	storage[noHouse] = "无房";
	storage[noHouse1] = "无房";
	storage[hasHouse] = "有房";
	storage[hasHouse1] = "有房有贷款";
	storage[hasHouse2] = "有房无贷款";
	storage[junior] = "中专及以上";
	storage[junior1] = "中专";
	storage[junior2] = "大专";
	storage[college] = "本科及以上";
	storage[college1] = "自考大学";
	storage[college2] = "三本";
	storage[college3] = "二本";
	storage[college4] = "一本";
	storage[college5] = "本科海外留学";
	storage[college6] = "本科双学位";
	storage[graduate] = "硕士及以上";
	storage[graduate1] = "学硕";
	storage[graduate2] = "专硕";
	storage[graduate3] = "硕士双学位";
	storage[graduate4] = "硕士海外留学";
	storage[philosophy] = "博士";
	storage[philosophy1] = "博士";
	storage[philosophy2] = "博士海外留学";
	storage[noMatter] = "无所谓";
	storage[maxPeople] = 10000;
}