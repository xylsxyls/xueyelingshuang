#pragma once
#include "StockMarketMacro.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** �洢gupiao��ʷhangqing
	@param [in] stock gupiao����
	@param [in] mysql ���ݿ�
	@param [in] file ���ص�ԭʼ�ļ�
	*/
	static void save(const std::string& stock, const std::string& file = "D:\\Table.txt");
};