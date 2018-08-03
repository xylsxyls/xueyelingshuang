#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include <string>

class NetHelper
{
public:
	static bool getSplitedBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength);

	static bool splitBuffer(char* buffer, int32_t length, int32_t& vernier, std::string& area, char*& tagBuffer, int32_t& tagLength);
};