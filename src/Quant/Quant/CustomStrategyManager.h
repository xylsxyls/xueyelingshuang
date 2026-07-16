#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include <vector>

/** 自定义策略管理器类
* 负责解析单个自定义策略推荐文件，并将推荐股票集合写入redis，单一实例
*/
class CustomStrategyManager
{
private:
	/** 构造函数
	*/
	CustomStrategyManager();

public:
	// key为交易日期，value为当天推荐股票集合
	typedef std::map<int32_t, std::vector<std::string>> StrategyStockMap;

	/** 获取单一实例
	@return 返回自定义策略管理器单一实例
	*/
	static CustomStrategyManager& instance();

	/** 获取策略推荐的全部股票
	@param [in] strategyName 策略名称
	@return 返回策略推荐股票集合
	*/
	std::vector<std::string> getStrategyStocks(const std::string& strategyName);

	/** 获取指定日期范围内策略推荐的全部股票
	@param [in] strategyName 策略名称
	@param [in] beginDate 开始日期
	@param [in] endDate 结束日期
	@return 返回策略推荐股票集合
	*/
	std::vector<std::string> getStrategyStocks(const std::string& strategyName, int32_t beginDate, int32_t endDate);

	/** 获取指定日期范围内策略按日期分组的推荐股票集合
	@param [in] strategyName 策略名称
	@param [in] beginDate 开始日期
	@param [in] endDate 结束日期
	@return 返回策略按日期分组的推荐股票集合
	*/
	StrategyStockMap getStrategyStockMap(const std::string& strategyName, int32_t beginDate, int32_t endDate);

	/** 保存单个策略推荐股票到redis
	@param [in] strategyName 策略名称
	*/
	void saveStrategyStocksToRedis(const std::string& strategyName);

private:
	/** 读取单个策略推荐股票配置文件
	@param [in] strategyName 策略名称
	@return 返回策略按日期分组的推荐股票集合
	*/
	StrategyStockMap readStrategyStockFile(const std::string& strategyName);

	/** 保存单个策略推荐股票到redis
	@param [in] strategyName 策略名称
	@param [in] strategyStockMap 策略按日期分组的推荐股票集合
	*/
	void saveStrategyStocksToRedis(const std::string& strategyName, const StrategyStockMap& strategyStockMap);

	/** 获取单个策略配置文件路径
	@param [in] strategyName 策略名称
	@return 返回单个策略配置文件路径
	*/
	std::string strategyConfigPath(const std::string& strategyName) const;

	/** 获取策略单日推荐股票集合redis键值
	@param [in] strategyName 策略名称
	@param [in] date 交易日期
	@return 返回策略单日推荐股票集合redis键值
	*/
	std::string strategyDateKey(const std::string& strategyName, const std::string& date) const;

	/** 获取策略单日推荐股票集合redis键值查询表达式
	@param [in] strategyName 策略名称
	@return 返回策略单日推荐股票集合redis键值查询表达式
	*/
	std::string strategyDateKeyPattern(const std::string& strategyName) const;

	/** 从策略单日推荐股票集合redis键值中解析日期
	@param [in] strategyName 策略名称
	@param [in] key redis键值
	@return 返回交易日期
	*/
	std::string dateFromStrategyDateKey(const std::string& strategyName, const std::string& key) const;
};
