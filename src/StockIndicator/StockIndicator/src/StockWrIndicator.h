#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>

class StockWr;

/** wr相关zhibiao
*/
class StockIndicatorAPI StockWrIndicator
{
public:
	/** 构造函数
	*/
	StockWrIndicator();

public:
	/** 设置redis原始数据
	@param [in] stock gupiao代码
	@param [in] vecIndicator zhibiao原始数据
	@param [in] dateIndex 日期所在索引值
	@param [in] indicatorIndex zhibiao所在索引值
	*/
	void setRedisData(const std::string& stock,
		const std::shared_ptr<std::vector<std::vector<std::string>>>& vecIndicator,
		int32_t dateIndex,
		const std::vector<int32_t>& indicatorIndex);

	/** 从原始数据加载到可用数据
	*/
	void load();

	/** 获取指定日期zhibiao
	@param [in] date 日期
	@return 返回指定日期zhibiao
	*/
	std::shared_ptr<StockWr> day(const IntDateTime& date);

	/** 清空
	*/
	void clear();

	/** 是否为空
	@return 返回是否为空
	*/
	bool empty();

protected:
	/** 计算衍生zhibiao
	*/
	void calc();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
	std::map<IntDateTime, std::shared_ptr<StockWr>> m_indicator;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_vecRedisIndicator;
	int32_t m_dateIndex;
	std::vector<int32_t> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};