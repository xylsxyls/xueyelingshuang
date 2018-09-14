#pragma once
#include <string>
#include <stdint.h>
#include "CBase64Macro.h"

class CBase64API CBase64
{
public:
	//这里必须是unsigned类型，否则编码中文的时候出错
	static std::string encode(const unsigned char* str, int32_t bytes);
	static std::string decode(const char* str, int32_t bytes);
};