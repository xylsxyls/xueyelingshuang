#pragma once
#include "TraceDataMapMacro.h"
#include <map>
#include <string>
#include <limits>

/** 调试值存储器
*/
template <typename TraceDataType>
class TraceDataMapAPI TraceDataMap
{
public:
	/** 添加一个值
	@param [in] key 值的含义
	@param [in] value 值
	*/
	void addValue(const std::string& key, TraceDataType value);

	/** 获取值的平均值
	@param [in] key 值的含义
	@param [in] value 值
	*/
	double average(const std::string& key) const;

	/** 获取值的最小值
	@param [in] key 值的含义
	@param [in] value 值
	*/
	TraceDataType min(const std::string& key) const;

	/** 获取值的最大值
	@param [in] key 值的含义
	@param [in] value 值
	*/
	TraceDataType max(const std::string& key) const;

private:
	struct Stats {
		//总和
		double m_sum;
		//计数
		int m_count;
		//最小值
		TraceDataType m_min;
		//最大值
		TraceDataType m_max;

		/** 构造函数
		*/
		Stats();
	};

	//存放数据的map
	std::map<std::string, Stats> m_data;
};

#include "TraceDataMap.inl"