#pragma once
#include <stdint.h>

class Util
{
public:
	static void getAllIndex(uint32_t allIndex, uint32_t& observeIndex, uint32_t& rangeIndex, uint32_t& transIndex,
		uint32_t offset);

	static std::vector<int32_t> groupToInt(const std::vector<std::string>& vecGroup);

	static std::vector<std::string> allField(bool isCreate);

	static std::vector<std::vector<int32_t>> getAllStockData(const std::string& stock, uint32_t beginTime, uint32_t endTime);
};