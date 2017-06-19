#include <SDKDDKVer.h>
#include "stdafx.h"
#include "ErrorInfo.h"

ErrorInfo errorInfo;

ErrorInfo::ErrorInfo(){
	storage[Success] = "核对成功";
	storage[NameError] = "名字的长度为2-5个字";
	storage[BirthError] = "生日为必填项目";
	storage[SexError] = "性别为必填项目";
	storage[MarriageError] = "婚否为必填项目";
	storage[EducationError] = "学历为必填项目";
}