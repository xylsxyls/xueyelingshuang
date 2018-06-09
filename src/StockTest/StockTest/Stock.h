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
    static void getPriceMap(MysqlCpp& mysql);

	//笔地图
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //获取图
    static void getPriceFromScreen();

    //获取自选股列表
    static std::vector<std::string> getStock();

    static void toPrec(std::string& result, int32_t prec);
};