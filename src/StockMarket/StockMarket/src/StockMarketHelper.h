#pragma once
#include "StockMarketMacro.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** 存储gupiao历史hangqing
	@param [in] stock gupiao代码
	@param [in] file 下载的原始文件
	*/
	static void saveMarketToMysql(const std::string& stock, const std::string& file);
};