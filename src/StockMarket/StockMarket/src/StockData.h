#pragma once
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>

class StockDay;
struct StockData
{
	//gupiao代码
	std::string m_stock;
	//gupiao名
	std::string m_name;
	//日期，kaigaodishou，索引对应history枚举
	std::map<IntDateTime, std::shared_ptr<StockDay>> m_history;
};