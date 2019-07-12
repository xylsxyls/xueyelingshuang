#pragma once
#include "StockMarketMacro.h"
#include <map>
#include "MysqlCpp/MysqlCppAPI.h"

class StockMarket;
/** hangqing管理
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
	/** 加载gupiao历史hangqing
	@param [in] 需要加载的gupiao集合，填空表示加载全部
	*/
	void load(const std::vector<std::string>& vecNeedStock = std::vector<std::string>());

	/** 存储gupiao历史hangqing
	@param [in] folder 存放gupiaohangqing的文件夹
	*/
	void save(const std::string& folder);

	/** 获取所有hangqing
	@return 返回所有hangqing
	*/
	std::map<std::string, std::shared_ptr<StockMarket>> allHistory();

	/** 获取所有gupiao
	@return 返回所有gupiao
	*/
	std::vector<std::string> allStock();

	/** 获取单个gupiaohangqing
	@param [in] stock gupiao代码
	@return 返回单个gupiaohangqing
	*/
	std::shared_ptr<StockMarket> history(const std::string& stock);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//所有hangqing
	std::map<std::string, std::shared_ptr<StockMarket>> m_allHistory;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	MysqlCpp m_mysql;
};