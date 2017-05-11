#include <SDKDDKVer.h>
#include "ErrorInfo.h"

ErrorInfo errorInfo;

ErrorInfo::ErrorInfo(){
	storage[Success] = "核对成功";
	storage[Name] = "名字的长度为2-5个字";
	storage[Birth] = "生日为必填项目";
	storage[Sex] = "性别为必填项目";
	storage[Marriage] = "婚否为必填项目";
	storage[Education] = "学历为必填项目";
}