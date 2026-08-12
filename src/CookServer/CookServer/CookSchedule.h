#pragma once
#include "CookModels.h"
#include <map>

/** 做菜排程，负责把多个菜谱步骤压成一条可执行时间线
*/
class CookSchedule
{
public:
	/** 根据多个菜谱ID计算一整顿饭的并行时间线
	@param [in] recipeIds 用户本次选择的菜谱ID列表
	@return 返回包含总耗时、操作时间、自由时间和步骤时间线的排程结果
	*/
	static PlanResult buildPlan(const std::vector<std::string>& recipeIds);

private:
	/** 判断步骤依赖是否已经全部完成
	@param [in] task 待判断步骤
	@param [in] done 已完成步骤表
	@return 返回true表示依赖已经满足
	*/
	static bool dependenciesDone(const CookTask& task, const std::map<std::string, ScheduledTask>& done);

	/** 计算依赖步骤最晚结束时间
	@param [in] task 待判断步骤
	@param [in] done 已完成步骤表
	@return 返回依赖步骤最晚结束秒数
	*/
	static int32_t dependenciesEnd(const CookTask& task, const std::map<std::string, ScheduledTask>& done);

	/** 判断两个操作区间之间的空档是否能算作自由时间
	@param [in] timeline 已生成的完整步骤时间线
	@param [in] gapStart 空档开始秒数
	@param [in] gapEnd 空档结束秒数
	@return 返回true表示这段时间足够长且用户可以离开厨房
	*/
	static bool gapCanBeFree(const std::vector<ScheduledTask>& timeline, int32_t gapStart, int32_t gapEnd);

	/** 将空档计入自由时间或边角时间，不可离开厨房的被动步骤会把空档切分成多段
	@param [in,out] plan 当前排程结果
	@param [in] timeline 已生成的完整步骤时间线
	@param [in] gapStart 空档开始秒数
	@param [in] gapEnd 空档结束秒数
	*/
	static void addGapToPlan(PlanResult& plan,
	                         const std::vector<ScheduledTask>& timeline,
	                         int32_t gapStart,
	                         int32_t gapEnd);

	/** 将已经切好的单段空档计入自由时间或边角时间
	@param [in,out] plan 当前排程结果
	@param [in] timeline 已生成的完整步骤时间线
	@param [in] gapStart 空档开始秒数
	@param [in] gapEnd 空档结束秒数
	*/
	static void addGapSegment(PlanResult& plan,
	                          const std::vector<ScheduledTask>& timeline,
	                          int32_t gapStart,
	                          int32_t gapEnd);
};