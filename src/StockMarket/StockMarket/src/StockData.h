#pragma once
#include <map>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>

class StockDay;
struct StockData
{
	//gupiao����
	std::string m_stock;
	//gupiao��
	std::string m_name;
	//���ڣ�kaigaodishou��������Ӧhistoryö��
	std::map<IntDateTime, std::shared_ptr<StockDay>> m_history;
};