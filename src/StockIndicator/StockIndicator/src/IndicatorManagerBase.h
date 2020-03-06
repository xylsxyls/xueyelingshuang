#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>
#include "StockIndicatorMacro.h"

struct IndicatorBase;

class StockIndicatorAPI IndicatorManagerBase
{
public:
	/** 构造函数
	*/
	IndicatorManagerBase();

	/** 析构函数
	*/
	virtual ~IndicatorManagerBase();

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
	virtual void load() = 0;

	/** 获取指定日期zhibiao
	@param [in] date 日期
	@return 返回指定日期zhibiao
	*/
	std::shared_ptr<IndicatorBase> day(const IntDateTime& date);

	/** 清空
	*/
	void clear();

	/** 是否为空
	@return 返回是否为空
	*/
	bool empty();

	/** 获取类型
	@return 返回类型
	*/
	std::string type();

protected:
	/** 计算衍生zhibiao
	*/
	virtual void calc() = 0;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_type;
	std::string m_stock;
	std::map<IntDateTime, std::shared_ptr<IndicatorBase>> m_indicator;
	std::shared_ptr<std::vector<std::vector<std::string>>> m_vecRedisIndicator;
	std::vector<int32_t> m_indicatorIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_dateIndex;
};