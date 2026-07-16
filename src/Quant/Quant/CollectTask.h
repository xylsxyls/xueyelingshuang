#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <atomic>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

enum class CollectMode
{
	CURRENT_STOCK = 1,
	STRATEGY_ALL = 2,
	STRATEGY_MISSING = 3
};

/** 股票采集任务类
* 负责采集股票数据，支持直接采集、按策略全量采集、按策略补缺采集
*/
class CollectTask : public CTask
{
public:
	/** 构造函数
	*/
	CollectTask();

public:
	/** 执行采集任务
	*/
	void DoTask();

	/** 停止采集任务
	*/
	void StopTask();

	/** 设置采集模式
	@param [in] mode 采集模式
	*/
	void setMode(CollectMode mode);

	/** 设置采集参数
	@param [in] count 采集次数，补缺采集时表示每只股票最多向前检查的次数
	@param [in] strategyName 策略名称，为空字符串时表示直接按count采集当前股票
	*/
	void setParam(int32_t count, const std::string& strategyName = "");

	/** 等待指定时间
	@param [in] time 等待时间
	*/
	void Sleep(int32_t time);

private:
	/** 采集单只股票
	@param [in] stock 股票代码
	@param [in] count 采集次数
	*/
	void collectStock(const std::string& stock, int32_t count);

	/** 按股票采集次数批量采集
	@param [in] stockCollectCount 股票采集次数集合
	*/
	void collectStocks(const std::map<std::string, int32_t>& stockCollectCount);

	/** 获取策略全部股票采集次数集合
	@param [in] strategyStockMap 策略按日期分组的推荐股票集合
	@param [in] count 每只股票采集次数
	@return 返回股票采集次数集合
	*/
	std::map<std::string, int32_t> strategyStockCollectCount(
		const std::map<int32_t, std::vector<std::string>>& strategyStockMap, int32_t count);

	/** 获取策略缺失股票采集次数集合
	@param [in] strategyStockMap 策略按日期分组的推荐股票集合
	@param [in] count 每只股票最多向前检查的次数
	@return 返回缺失数据对应的股票采集次数集合
	*/
	std::map<std::string, int32_t> missingStockCollectCount(
		const std::map<int32_t, std::vector<std::string>>& strategyStockMap, int32_t count);

	/** 检查股票指定日期数据是否已存在
	@param [in] stock 股票代码
	@param [in] date 日期
	@return 返回是否已存在
	*/
	bool hasStockDateData(const std::string& stock, int32_t date);

	/** 切换采集股票
	@param [in] fromStock 当前股票代码
	@param [in] toStock 目标股票代码
	*/
	void switchStock(const std::string& fromStock, const std::string& toStock);

private:
	// 等待事件
	Semaphore m_sleep;
	// 每只股票采集次数
	int32_t m_count;
	// 自定义策略名称
	std::string m_strategyName;
	// 采集模式
	CollectMode m_mode;
	// 是否退出任务
	std::atomic<bool> m_exit;
};
