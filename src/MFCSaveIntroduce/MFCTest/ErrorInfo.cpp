#include <SDKDDKVer.h>
#include "ErrorInfo.h"

ErrorInfo errorInfo;

ErrorInfo::ErrorInfo(){
	storage[Success] = string("核对成功");
	storage[Name] = string("名字的长度为2-5个字");
	storage[Birth] = string("生日为必填项目");
	storage[Sex] = string("性别为必填项目");
	storage[Marriage] = string("婚否为必填项目");
	storage[Education] = string("学历为必填项目");
}