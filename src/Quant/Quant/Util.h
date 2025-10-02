#pragma once
#include <stdint.h>
#include "Config.h"

class Util
{
public:
	static void getAllIndex(uint32_t allIndex, uint32_t& observeIndex, uint32_t& rangeIndex, uint32_t& transIndex,
		uint32_t offset);

	static std::vector<int32_t> groupToInt(const std::vector<std::string>& vecGroup);

	static std::vector<std::string> allField(bool isCreate);

	static std::vector<std::vector<int32_t>> getAllStockData(const std::string& stock, uint32_t beginTime, uint32_t endTime);

	/** 根据Overall枚举值获取对应的行情数组下标值
	@param [in] overall Overall枚举值
	@return 返回对应的数组下标值
	*/
	static int32_t getOverallIndex(Overall overall);

	/** 根据行情数组下标值获取对应的Overall枚举值
	@param [in] index 数组下标值
	@return 返回对应的Overall枚举值，如果找不到返回COUNT
	*/
	static Overall getOverallFromIndex(int32_t index);

	/** 根据ObserveTime枚举值获取对应的行情数组下标值（相对于Overall之后的位置）
	@param [in] observeTime 观察时间点枚举
	@return 返回对应的数组下标值
	*/
	static int32_t getObserveTimeIndex(ObserveTime observeTime);

	/** 根据行情数组下标值获取对应的ObserveTime枚举值
	@param [in] index 数组下标值（相对于Overall之后的位置）
	@return 返回对应的ObserveTime枚举值，如果找不到返回COUNT
	*/
	static ObserveTime getObserveTimeFromIndex(int32_t index);

	/** 根据ObserveTime、RangeTime和TransType枚举值获取对应的三维数组展平下标值
	@param [in] observeTime 观察时间点枚举
	@param [in] rangeTime 时间范围枚举
	@param [in] transType 交易类型枚举
	@return 返回对应的三维数组展平后的下标值
	*/
	static int32_t getPriceMatrixIndex(ObserveTime observeTime, RangeTime rangeTime, TransType transType);

	/** 根据三维数组展平下标值获取对应的ObserveTime、RangeTime和TransType枚举值
	@param [in] index 三维数组展平后的下标值
	@param [out] observeTime 返回的观察时间点枚举
	@param [out] rangeTime 返回的时间范围枚举
	@param [out] transType 返回的交易类型枚举
	@return 返回是否成功找到对应的枚举值
	*/
	static bool getEnumsFromPriceIndex(int32_t index, ObserveTime& observeTime, RangeTime& rangeTime, TransType& transType);

	/** 获取总字段数量（用于验证数组大小）
	@return 返回总字段数量
	*/
	static int32_t getTotalFieldCount();
};