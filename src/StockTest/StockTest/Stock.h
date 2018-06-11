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
	static std::vector<std::string> getPriceMap(MysqlCpp& mysql, int32_t& useCount);

	//笔地图
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //获取图
    static void getPriceFromScreen(const std::string& stock);

    //获取自选
    static std::vector<std::string> getSelfStock();

    static void toPrec(std::string& result, int32_t prec);
};