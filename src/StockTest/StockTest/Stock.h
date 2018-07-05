#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>
#include "BigNumber/BigNumberAPI.h"

class Stock
{
public:
	//笔入库
	static bool insertDatabase(MysqlCpp& mysql);

	//均价图
    static std::map<std::string, std::vector<BigNumber>> getPriceMap(MysqlCpp& mysql, int32_t& useCount, BigNumber& reserveValue);

	//均价图
    static std::map<std::string, std::vector<BigNumber>> getPriceMapFromLocal(int32_t& useCount, BigNumber& reserveValue);

	//笔地图
	static std::map<int32_t, std::string> getFundMap(MysqlCpp& mysql);

    //获取图
    static void getPriceFromScreen(const std::string& stock);

    //获取自选
    static std::vector<std::vector<std::string>> getSelfStock(MysqlCpp& mysql, int32_t zubie = 0);

	//获取自定义
	static std::vector<std::vector<std::string>> getDefineStock(const std::string& define);

	//数据转换为均价图
    static std::map<std::string, std::vector<BigNumber>> priceMap(const std::vector<std::vector<std::string>>& result, int32_t& useCount, BigNumber& reserveValue);

	//获取处理好的笔数据
	static std::vector<std::vector<std::string>> getResultVecFromMysql(MysqlCpp& mysql);

	//获取处理好的笔数据
	static std::vector<std::vector<std::string>> getResultVec();

	//存入当天价格
	static void insertQuoteDataBase(MysqlCpp& mysql);

    //回测，需要先运行insertQuoteDataBase
    static void chooseTest(MysqlCpp& mysql, const std::string& preDate);

    //获取上一天的时间
    static std::string getPreDate(MysqlCpp& mysql);

    static void toPrec(std::string& result, int32_t prec);

	static void printReserveMap(const std::map<BigNumber, std::vector<std::string>>& reserveMap);

    static void printChooseMap(const std::map<BigNumber, std::vector<BigNumber>>& chooseMap);

	//分笔入库
	static void fenbiInsertDataBase(MysqlCpp& mysql, const std::string& stockNum);

    //保存选择
    static void saveChooseToDataBase(MysqlCpp& mysql, std::map<BigNumber, std::vector<BigNumber>>& chooseMap, int32_t zubie);

    //打印均价图
    static void printPriceMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

    //添加到选择图
    //xiangcha,daima,xiaodan,dadan
    static void addChooseMap(std::map<BigNumber, std::vector<BigNumber>>& chooseMap, const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::string& stockNum);
};