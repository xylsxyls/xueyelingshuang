#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>

class Stock
{
public:
	//笔入库
	static void insertDatabase(MysqlCpp& mysql);

	//均价图
	static std::map<int32_t, std::string> getPriceMap(MysqlCpp& mysql);

	//笔地图
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);
};