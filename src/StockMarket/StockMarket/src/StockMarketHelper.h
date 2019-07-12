#pragma once
#include "StockMarketMacro.h"

class StockMarketAPI StockMarketHelper
{
public:
	/** 存储gupiao历史hangqing
	@param [in] stock gupiao代码
	@param [in] mysql 数据库
	@param [in] file 下载的原始文件
	*/
	static void save(const std::string& stock, const std::string& file = "D:\\Table.txt");
};