#pragma once
#include <stdint.h>
#include "Config.h"
#include <map>
#include <vector>
#include <memory>

struct StrategyResult;

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

	/** 计算当前索引对应的时间
	@param [in] dayInfoIndex 当前索引值
	@return 返回对应时间
	*/
	static ObserveTime indexToTime(uint32_t dayInfoIndex);

	/** 计算最佳价格索引值
	@param [in] time 当前时间
	@param [in] range 范围
	@param [in] trans 交易方式
	@return 返回最佳价格索引值
	*/
	static int32_t bestPrice(ObserveTime time, RangeTime range, TransType trans);

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

	/** 将观察时间枚举转换为钟表字符串表示
	@param [in] time 观察时间枚举值
	@return 对应的钟表字符串名称
	*/
	static std::string observeTimeToWatchString(ObserveTime time);

	/** 获取观察时间的数值表示
	@param [in] time 观察时间枚举值
	@return 对应的整数值
	*/
	static int32_t getTimeValue(ObserveTime time);

	/** 获取时间枚举值
	@param [in] timeValue 实际时间值
	@return 返回时间枚举值
	*/
	static ObserveTime getObserveTime(int32_t timeValue);

	/** 获取时间范围的数值表示
	@param [in] range 时间范围枚举值
	@return 对应的整数值
	*/
	static int32_t getRangeValue(RangeTime range);

	/** 计算范围结束时间
	@param [in] startTime 开始时间
	@param [in] addMinutes 增加的时间（分钟）
	@return 结束时间
	*/
	static int32_t rangeEndTime(int32_t startTime, int32_t addMinutes);

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

	/** 计算下一个未来周期的结束时间
	@param [in] beginTime 本周期开始时间
	@param [in] beginTime 本周期结束时间
	@return 下一个未来周期的结束时间
	*/
	static uint32_t calcFutureEndTime(uint32_t beginTime, uint32_t endTime);

	/** 计算上一个历史周期的开始时间
	@param [in] beginTime 本周期开始时间
	@param [in] beginTime 本周期结束时间
	@return 上一个历史周期的开始时间
	*/
	static uint32_t calcHistoryBeginTime(uint32_t beginTime, uint32_t endTime);

	/** 排列组合出所有参数组合
	@param [in] allParam 每个参数可能出现的值
	@return 返回所有参数组合
	*/
	static std::vector<std::vector<int32_t>> combinatoricsToAllParam(const std::vector<std::vector<int32_t>>& allParam);

	/** 让原始检测数据和验证数据合成一个map
	@param [in] detectMap 原始检测数据
	@param [in] verifyMap 验证数据
	@return 返回合成map
	*/
	static std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> synthesisMap(
		const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap,
		const std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>& verifyMap);
};