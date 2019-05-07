#pragma once
#include "StockIndexMacro.h"
#include <memory>
#include "MysqlCpp/MysqlCppAPI.h"
#include <map>

class StockIndex;
class StockIndexAPI StockAllIndex
{
private:
	/** 构造函数
	*/
	StockAllIndex();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockAllIndex& instance();

protected:
	/** 初始化
	*/
	void init();

public:
	/** 获取指标
	@param [in] stock 股票代码
	@return 返回指标
	*/
	std::shared_ptr<StockIndex> index(const std::string& stock) const;

	/** 获取所有指标
	@return 返回所有指标
	*/
	std::map<std::string, std::shared_ptr<StockIndex>> allIndex() const;

	/** 指标存储
	*/
	void save() const;

	/** 加载
	*/
	void load();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::shared_ptr<StockIndex>> m_stockAllIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};