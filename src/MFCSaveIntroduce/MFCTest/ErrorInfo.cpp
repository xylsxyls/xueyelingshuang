#include <SDKDDKVer.h>
#include "ErrorInfo.h"

ErrorInfo errorInfo;

ErrorInfo::ErrorInfo(){
	storage[Success] = string("核对成功");
	storage[NameError] = string("名字的长度为2-5个字");
	storage[BirthError] = string("生日为必填项目");
	storage[SexError] = string("性别为必填项目");
	storage[MarriageError] = string("婚否为必填项目");
	storage[EducationError] = string("学历为必填项目");
}