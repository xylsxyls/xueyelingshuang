#pragma once
#include <stdint.h>
#include "Config.h"

class Util
{
public:
	/** 从组合索引中分解出各个维度的索引
	@param [in] allIndex 组合索引值
	@param [out] observeIndex 观察时间维度索引
	@param [out] rangeIndex 时间范围维度索引
	@param [out] transIndex 交易类型维度索引
	@param [in] offset 索引偏移量
	*/
	static void getAllIndex(uint32_t allIndex, uint32_t& observeIndex, uint32_t& rangeIndex, uint32_t& transIndex,
		uint32_t offset);

	/** 将字符串组转换为整数组
	@param [in] vecGroup 输入字符串组，格式为"prefix_number"
	@param [in] origin 从0开始的偏移量
	@return 提取的数字部分组成的整数组
	*/
	static std::vector<int32_t> groupToInt(const std::vector<std::string>& vecGroup, int32_t origin);

	/** 将整数金额（分）转换为带小数点的字符串金额（元）
	@param [in] amount 整数金额，单位为分
	@return 带小数点的字符串金额，单位为元
	*/
	static std::string transYuan(int32_t amount);

	/** 生成所有字段名称列表
	@param [in] isCreate 是否为创建模式，true时在字段名后添加" integer"类型声明
	@return 字段名称字符串列表
	*/
	static std::vector<std::string> allField(bool isCreate);

	/** 获取指定股票在时间范围内的所有数据
	@param [in] stock 股票代码
	@param [in] beginTime 开始时间戳
	@param [in] endTime 结束时间戳
	@return 二维整型数组，外层按时间点排序，内层包含所有字段的数值
	*/
	static std::vector<std::vector<int32_t>> getAllStockData(const std::string& stock, uint32_t beginTime, uint32_t endTime);

	/** 将观察时间枚举转换为字符串表示
	@param [in] time 观察时间枚举值
	@return 对应的字符串名称
	*/
	static std::string observeTimeToString(ObserveTime time);

	/** 将观察时间字符串转换为枚举表示
	@param [in] strTime 观察时间字符串
	@return 对应的枚举值
	*/
	static ObserveTime observeStringToTime(const std::string& strTime);

	/** 获取观察时间的数值表示
	@param [in] time 观察时间枚举值
	@return 对应的整数值
	*/
	static int32_t getTimeValue(ObserveTime time);

	/** 获取时间范围的数值表示
	@param [in] range 时间范围枚举值
	@return 对应的整数值
	*/
	static int32_t getRangeValue(RangeTime range);

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