#pragma once
#include "StockMarketMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** �洢gupiao��ʷhangqing
	@param [in] stock gupiao����
	@param [in] file ���ص�ԭʼ�ļ�
	*/
	static void saveMarketToMysql(const std::string& stock, const std::string& file);

	/** ���µ���hangqing
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] market ����hangqing
	*/
	static void updateDateMarketToMysql(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);

	/** ���µ���hangqing��redis
	@param [in] stock gupiao����
	@param [in] date ����
	@param [in] market ����hangqing
	*/
	static void updateDateMarketToRedis(const std::string& stock, const IntDateTime& date, const std::vector<std::string>& market);
};