#pragma once
#include "CompetitionManager.h"

/** 验证管理类
*/
class VerifyManager
{
private:
	/** 构造函数
	*/
	VerifyManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static VerifyManager& instance();

public:
	/** 验证历史
	@param [in] mode 策略模式
	@param [in] times 验证次数
	@param [in] originalConfig 竞赛配置
	@return 返回验证结果，收益分，一次结果为一列，第一列是当前策略结果，同一受益分有多个，第二列是历史第一次验证结果
	*/
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> verifyHistory(
		StrategyMode mode, uint32_t times, const CompetitionConfig& originalConfig);

	/** 验证未来
	@param [in] mode 策略模式
	@param [in] times 验证次数
	@param [in] originalConfig 竞赛配置
	@return 返回验证结果，收益分，一次结果为一列，第一列是当前策略结果，同一受益分有多个，第二列是未来第一次验证结果
	*/
	std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>> verifyFuture(
		StrategyMode mode, uint32_t times, const CompetitionConfig& originalConfig);

	/** 验证历史未来
	@param [in] mode 策略模式
	@param [in] historyTimes 验证历史次数
	@param [in] futureTimes 验证未来次数
	@param [in] originalConfig 竞赛配置
	@return 返回验证结果，收益分，一次结果为一列，第一列是当前策略结果，第二列是未来第一次验证结果，first历史second未来
	*/
	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> verifyHistoryFuture(
		StrategyMode mode, uint32_t historyTimes, uint32_t futureTimes, const CompetitionConfig& originalConfig);

	/** 打印验证结果
	@param [in] detectMap 验证检测结果
	@param [in] showRank 是否显示前10名具体收益值
	@param [in] showAvg 是否显示大类排名的每一类平均收益
	*/
	void printDetectMap(const std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>& detectMap,
		bool showRank = false, bool showAvg = false);

	/** 获取验证结果
	@return 返回验证结果
	*/
	const std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>>& getResult();

	/** 计算平均收益
	@param [in] avgAnnualReturnMap 平均收益map，排名，内部为一大类的收益总和和次数
	@return 一列为一次验证，内部为一大类的收益总和和次数
	*/
	std::vector<std::vector<std::pair<BigNumber, uint32_t>>> calcAvgMap(
		const std::map<uint32_t, std::vector<std::pair<BigNumber, uint32_t>>>& avgAnnualReturnMap);

private:
	std::pair<std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>,
		std::map<int32_t, std::vector<std::vector<std::shared_ptr<StrategyResult>>>>> m_result;
};