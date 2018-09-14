#pragma once
#include <string>
#include <stdint.h>
#include "CBase64Macro.h"

class CBase64API CBase64
{
public:
	//���������unsigned���ͣ�����������ĵ�ʱ�����
	static std::string encode(const unsigned char* str, int32_t bytes);
	static std::string decode(const char* str, int32_t bytes);
};