#pragma once
#include <vector>
#include <map>
#include <string>
#include <memory>

class Market
{
public:
	/** 构造函数
	*/
	Market();

	/** 初始化市场数据
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void init(uint32_t beginTime, uint32_t endTime);

	/** 添加股票到市场数据
	@param [in] stock 股票代码
	*/
	void addStock(const std::string& stock);

	/** 获取股票在指定日期的数据
	@param [in] stock 股票代码
	@param [in] date 日期
	@return 返回该日期的行情数据向量，如果找不到返回空向量
	*/
	const std::vector<int32_t>& getStockData(const std::string& stock, uint32_t date) const;

	/** 获取股票在所有日期的数据
	@param [in] stock 股票代码
	@return 返回所有日期的行情数据向量，如果找不到返回空向量
	*/
	const std::vector<std::vector<int32_t>>& getAllStockData(const std::string& stock) const;

	/** 获取开始时间
	@return 返回开始时间
	*/
	uint32_t getBeginTime() const;

	/** 获取结束时间
	@return 返回结束时间
	*/
	uint32_t getEndTime() const;

	/** 检查股票是否存在
	@param [in] stock 股票代码
	@return 返回是否存在
	*/
	bool hasStock(const std::string& stock) const;

private:
	// 股票数据映射：股票代码 -> 日期数据向量
	std::map<std::string, std::vector<std::vector<int32_t>>> m_stockData;

	// 日期索引映射：股票代码 -> 日期到索引的映射
	std::map<std::string, std::map<uint32_t, size_t>> m_dateIndex;

	uint32_t m_beginTime;
	uint32_t m_endTime;
	std::vector<int32_t> m_emptyDayInfo;
	std::vector<std::vector<int32_t>> m_emptyStockData;
};