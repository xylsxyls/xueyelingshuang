#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "MysqlCpp/MysqlCppAPI.h"

class StockMarket;
/** 行情管理
*/
class StockMarketAPI StockAllMarket
{
private:
	/** 构造函数
	*/
	StockAllMarket();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockAllMarket& instance();

protected:
	/** 初始化
	*/
	void init();

public:
	/** 加载股票历史行情
	@param [in] 需要加载的股票集合，填空表示加载全部
	*/
	void load(const std::vector<std::string>& vecNeedStock = std::vector<std::string>());

	/** 存储股票历史行情
	@param [in] folder 存放股票行情的文件夹
	*/
	void save(const std::string& folder);

	/** 获取所有行情
	@return 返回所有行情
	*/
	std::map<std::string, std::shared_ptr<StockMarket>> allHistory();

	/** 获取所有股票
	@return 返回所有股票
	*/
	std::vector<std::string> allStock();

	/** 获取单个股票行情
	@param [in] stock 股票代码
	@return 返回单个股票行情
	*/
	std::shared_ptr<StockMarket> history(const std::string& stock);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//所有行情
	std::map<std::string, std::shared_ptr<StockMarket>> m_allHistory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};