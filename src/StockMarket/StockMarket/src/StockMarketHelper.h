#pragma once
#include "StockMarketMacro.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** �洢gupiao��ʷhangqing
	@param [in] stock gupiao����
	@param [in] file ���ص�ԭʼ�ļ�
	*/
	static void saveMarketToMysql(const std::string& stock, const std::string& file);
};