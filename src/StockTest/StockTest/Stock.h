#pragma once
#include "MysqlCpp/MysqlCppAPI.h"
#include <string>
#include <map>
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class Stock
{
public:
	//笔入库
	static bool insertDatabase(MysqlCpp& mysql);
	
	//从数据库获取资金图
	static std::map<std::string, std::vector<BigNumber>> getCapitalMapFromDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi, const std::string& stockNum, const std::string& date = "");

    //本地获取资金图
    static std::map<std::string, std::vector<BigNumber>> getCapitalMapFromLocal(MysqlCpp& mysql, const std::string& stockNum, const std::string& date = "");

    //获取资金图
    static std::map<std::string, std::vector<BigNumber>> getCapitalMap(MysqlCpp& mysql, const std::vector<std::vector<std::string>>& fenbiVec, const std::string& stockNum, const std::string& date = "");

	//均价图
    static std::map<std::string, std::vector<BigNumber>> getPriceMapFromDataBase(MysqlCpp& mysql, const std::string& stockNum, int32_t& useCount, BigNumber& reserveValue, const std::string& date = "");

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
    static std::vector<std::vector<std::string>> getResultVecFromMysql(MysqlCpp& mysql, const std::string& stockNum, const std::string& date);

	//获取处理好的笔数据
	static std::vector<std::vector<std::string>> getResultVecFromLocal();

	//存入当天价格
	static void insertQuoteDataBase(MysqlCpp& mysql);

    //回测，需要先运行insertQuoteDataBase
    static void chooseTest(MysqlCpp& mysql, const std::string& preDate);

    //获取上一天的时间
	static std::string getPreDate(MysqlCpp& mysql, const std::string& date = "");

	//获取下一天的时间，前提已经存入stockquote
	static std::string getNextDate(MysqlCpp& mysql, const std::string date = "");

    static void toPrec(std::string& result, int32_t prec);

	static void printReserveMap(const std::map<BigNumber, std::vector<std::string>>& reserveMap);

    static void printChooseMap(const std::map<BigNumber, std::vector<BigNumber>>& chooseMap);

	//分笔入库
	static void fenbiInsertDataBase(MysqlCpp& mysql, const std::string& stockNum);

	//分笔验证，总数据
	static std::map<std::string, BigNumber> getLastPriceFromStockQuote(MysqlCpp& mysql, const std::string& date);

	//分笔验证，分笔数据
	static std::map<std::string, BigNumber> getLastPriceFromStockTradeQuote(MysqlCpp& mysql, MysqlCpp& mysqlfenbi, const std::string& date);

    //保存选择
    static void saveChooseToDataBase(MysqlCpp& mysql, std::map<BigNumber, std::vector<BigNumber>>& chooseMap, int32_t zubie);

    //保存资金选择
    static void saveCapitalChooseToDataBase(MysqlCpp& mysql, std::map<IntDateTime, std::map<std::string, std::vector<BigNumber>>>& chooseMap, int32_t zubie);

    //删除部分资金选择
    static void deleteCapitalChooseToDataBase(MysqlCpp& mysql);

    //打印图
	static void printMap(const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::vector<std::string>& vecPrint, const std::vector<std::vector<std::string>>& sep);

	//打印价格图
	static void printPriceMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

	//打印资金图
	static void printCapitalMap(const std::map<std::string, std::vector<BigNumber>>& priceMap);

	//最好分析
	static void bestAnalyzeDataBase(MysqlCpp& mysql, MysqlCpp& mysqlfenbi);

	//从资金图中选取
	static std::map<std::string, std::vector<BigNumber>> chooseFromCapitalMap(const std::map<std::string, std::map<std::string, std::vector<BigNumber>>>& capitalMap);

	//打印选择的资金图
	static void printChooseFromCapitalMap(const std::map<std::string, std::vector<BigNumber>>& capitalMap);

    //估算收益
    static BigNumber reckonGain(const std::map<IntDateTime, std::map<std::string, std::vector<BigNumber>>>& chooseMapAll);

    //添加到选择图
    //xiangcha,daima,xiaodan,dadan
    static void addChooseMap(std::map<BigNumber, std::vector<BigNumber>>& chooseMap, const std::map<std::string, std::vector<BigNumber>>& priceMap, const std::string& stockNum);
};