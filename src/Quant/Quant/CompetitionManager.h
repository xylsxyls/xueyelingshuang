#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Strategy.h"
#include "Market.h"
#include "Semaphore/SemaphoreAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <memory>
#include <string>
#include "CompetitionTask.h"

// 前向声明
class StrategyTask;

/** 竞赛管理器类
* 负责管理策略竞赛的整个生命周期，单一实例
*/
class CompetitionManager
{
private:
	/** 构造函数
	*/
	CompetitionManager();

public:
	/** 获取单一实例
	@return 返回竞赛管理器单一实例
	*/
	static CompetitionManager& instance();

	/** 析构函数
	*/
	~CompetitionManager();

	/** 初始化竞赛管理器
	*/
	void init();

	/** 反初始化竞赛管理器
	*/
	void uninit();

	/** 添加竞赛
	@param [in] strategyMode 策略模式
	@param [in] config 竞赛配置
	@return 返回是否开始成功
	*/
	void addCompetition(StrategyMode strategyMode, const CompetitionConfig& config);

	/** 是否显示结果
	@param [in] isShowResult 是否显示结果
	@param [in] isShowTradeLog 是否展示交易记录日志
	*/
	void setParam(bool isShowResult, bool isShowTradeLog);

	/** 开始竞赛
	@param [in] config 竞赛配置
	@param [in] strategyMode 策略模式
	@param [in] allParams 所有策略参数组合
	@return 返回是否开始成功
	*/
	bool startCompetition();

	/** 停止竞赛
	*/
	void stopCompetition();

	/** 等待处理好的竞赛类
	@return 返回运行好的竞赛类
	*/
	std::shared_ptr<CompetitionTask> waitForEnd();

	/** 检测竞赛配置是否正确
	@param [in] strategyMode 策略模式
	@param [in] config 竞赛配置
	@return 返回竞赛配置是否正确
	*/
	bool isCompetitionParamValid(StrategyMode strategyMode, const CompetitionConfig& config);

private:
	// 处理竞赛任务线程
	uint32_t m_competitionThreadId;
	// 是否显示结果
	bool m_isShowResult;
	// 是否展示交易记录日志
	bool m_isShowTradeLog;
	// 所有竞赛任务参数
	std::map<int32_t, CompetitionConfig> m_competitionConfigMap;
	// 竞赛类
	std::shared_ptr<CompetitionTask> m_spCompetitionTask;
	// 是否已经初始化
	std::atomic<bool> m_isInit;
};