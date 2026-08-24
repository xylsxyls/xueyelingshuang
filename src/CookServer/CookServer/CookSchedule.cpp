#include "CookSchedule.h"
#include "CookCatalog.h"
#include "Config.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>

bool CookSchedule::dependenciesDone(const CookTask& task, const std::map<std::string, ScheduledTask>& done)
{
	for (size_t i = 0; i < task.m_dependencies.size(); ++i)
	{
		if (done.find(task.m_dependencies[i]) == done.end())
		{
			return false;
		}
	}
	return true;
}

int32_t CookSchedule::dependenciesEnd(const CookTask& task, const std::map<std::string, ScheduledTask>& done)
{
	int32_t result = 0;
	for (size_t i = 0; i < task.m_dependencies.size(); ++i)
	{
		std::map<std::string, ScheduledTask>::const_iterator it = done.find(task.m_dependencies[i]);
		if (it != done.end())
		{
			result = std::max(result, it->second.m_endSeconds);
		}
	}
	return result;
}

bool CookSchedule::isFreeBackgroundWait(const CookTask& task)
{
	if (task.m_active)
	{
		return false;
	}
	if (task.m_backgroundWaitMode == "free")
	{
		return true;
	}
	if (task.m_backgroundWaitMode == "watch")
	{
		return false;
	}
	return task.m_canLeaveKitchen && task.m_safetyLevel != "attention" && task.m_safetyLevel != "danger";
}

bool CookSchedule::gapCanBeFree(const std::vector<ScheduledTask>& timeline, int32_t gapStart, int32_t gapEnd)
{
	bool hasLeaveWindow = false;
	bool hasAttentionWait = false;
	for (size_t i = 0; i < timeline.size(); ++i)
	{
		const ScheduledTask& task = timeline[i];
		if (task.m_active)
		{
			continue;
		}
		if (task.m_startSeconds < gapEnd && task.m_endSeconds > gapStart)
		{
			if (CookSchedule::isFreeBackgroundWait(task))
			{
				hasLeaveWindow = true;
			}
			else
			{
				hasAttentionWait = true;
			}
		}
	}
	return gapEnd - gapStart >= g_config.m_freeBlockMinimumSeconds && hasLeaveWindow && !hasAttentionWait;
}

void CookSchedule::addGapSegment(PlanResult& plan,
                                 const std::vector<ScheduledTask>& timeline,
                                 int32_t gapStart,
                                 int32_t gapEnd)
{
	if (gapEnd <= gapStart)
	{
		return;
	}
	if (CookSchedule::gapCanBeFree(timeline, gapStart, gapEnd))
	{
		FreeBlock block;
		block.m_startSeconds = gapStart;
		block.m_endSeconds = gapEnd;
		plan.m_freeBlocks.push_back(block);
		plan.m_freeSeconds += gapEnd - gapStart;
	}
	else
	{
		plan.m_edgeSeconds += gapEnd - gapStart;
	}
}

void CookSchedule::addGapToPlan(PlanResult& plan,
                                const std::vector<ScheduledTask>& timeline,
                                int32_t gapStart,
                                int32_t gapEnd)
{
	if (gapEnd <= gapStart)
	{
		return;
	}

	std::vector<std::pair<int32_t, int32_t>> blockers;
	for (size_t i = 0; i < timeline.size(); ++i)
	{
		const ScheduledTask& task = timeline[i];
		if (task.m_active || CookSchedule::isFreeBackgroundWait(task))
		{
			continue;
		}
		if (task.m_startSeconds < gapEnd && task.m_endSeconds > gapStart)
		{
			blockers.push_back(std::make_pair(std::max(task.m_startSeconds, gapStart), std::min(task.m_endSeconds, gapEnd)));
		}
	}
	std::sort(blockers.begin(), blockers.end());

	int32_t cursor = gapStart;
	for (size_t i = 0; i < blockers.size(); ++i)
	{
		int32_t blockStart = blockers[i].first;
		int32_t blockEnd = blockers[i].second;
		if (blockEnd <= cursor)
		{
			continue;
		}
		CookSchedule::addGapSegment(plan, timeline, cursor, blockStart);
		plan.m_edgeSeconds += blockEnd - std::max(cursor, blockStart);
		cursor = blockEnd;
	}
	CookSchedule::addGapSegment(plan, timeline, cursor, gapEnd);
}

void CookSchedule::schedulePreparedPlan(PlanResult& plan, const std::vector<CookTask>& tasks)
{
	std::set<std::string> scheduledIds;
	std::map<std::string, ScheduledTask> done;
	std::map<std::string, int32_t> resourceAvailable;
	int32_t userAvailable = 0;

	std::map<std::string, const CookTask*> taskById;
	std::map<std::string, std::vector<std::string>> childrenByTaskId;
	for (size_t i = 0; i < tasks.size(); ++i)
	{
		taskById[tasks[i].m_id] = &tasks[i];
		for (size_t j = 0; j < tasks[i].m_dependencies.size(); ++j)
		{
			childrenByTaskId[tasks[i].m_dependencies[j]].push_back(tasks[i].m_id);
		}
	}

	std::map<std::string, int32_t> waitScoreCache;
	std::map<std::string, int32_t> criticalScoreCache;
	std::set<std::string> waitScoreVisiting;
	std::set<std::string> criticalScoreVisiting;

	// waitScore入参：taskId是待评分步骤ID。
	// waitScore出参：无。
	// waitScore返回值：返回该步骤之后能尽早启动的后台等待链秒数。
	std::function<int32_t(const std::string&)> waitScore = [&](const std::string& taskId) -> int32_t
	{
		std::map<std::string, int32_t>::const_iterator cacheIt = waitScoreCache.find(taskId);
		if (cacheIt != waitScoreCache.end())
		{
			return cacheIt->second;
		}
		if (waitScoreVisiting.find(taskId) != waitScoreVisiting.end())
		{
			return 0;
		}
		std::map<std::string, const CookTask*>::const_iterator taskIt = taskById.find(taskId);
		if (taskIt == taskById.end() || taskIt->second == nullptr)
		{
			return 0;
		}

		waitScoreVisiting.insert(taskId);
		const CookTask& task = *taskIt->second;
		int32_t bestChildScore = 0;
		std::map<std::string, std::vector<std::string>>::const_iterator childrenIt = childrenByTaskId.find(taskId);
		if (childrenIt != childrenByTaskId.end())
		{
			for (size_t i = 0; i < childrenIt->second.size(); ++i)
			{
				bestChildScore = std::max(bestChildScore, waitScore(childrenIt->second[i]));
			}
		}
		int32_t score = bestChildScore;
		if (!task.m_active && (CookSchedule::isFreeBackgroundWait(task) || task.m_continuesDuringPause))
		{
			score += task.m_durationSeconds;
		}
		waitScoreVisiting.erase(taskId);
		waitScoreCache[taskId] = score;
		return score;
	};

	// criticalScore入参：taskId是待评分步骤ID。
	// criticalScore出参：无。
	// criticalScore返回值：返回从该步骤开始的最长依赖链秒数。
	std::function<int32_t(const std::string&)> criticalScore = [&](const std::string& taskId) -> int32_t
	{
		std::map<std::string, int32_t>::const_iterator cacheIt = criticalScoreCache.find(taskId);
		if (cacheIt != criticalScoreCache.end())
		{
			return cacheIt->second;
		}
		if (criticalScoreVisiting.find(taskId) != criticalScoreVisiting.end())
		{
			return 0;
		}
		std::map<std::string, const CookTask*>::const_iterator taskIt = taskById.find(taskId);
		if (taskIt == taskById.end() || taskIt->second == nullptr)
		{
			return 0;
		}

		criticalScoreVisiting.insert(taskId);
		const CookTask& task = *taskIt->second;
		int32_t bestChildScore = 0;
		std::map<std::string, std::vector<std::string>>::const_iterator childrenIt = childrenByTaskId.find(taskId);
		if (childrenIt != childrenByTaskId.end())
		{
			for (size_t i = 0; i < childrenIt->second.size(); ++i)
			{
				bestChildScore = std::max(bestChildScore, criticalScore(childrenIt->second[i]));
			}
		}
		int32_t score = task.m_durationSeconds + bestChildScore;
		criticalScoreVisiting.erase(taskId);
		criticalScoreCache[taskId] = score;
		return score;
	};

	while (scheduledIds.size() < tasks.size())
	{
		int32_t bestIndex = -1;
		int32_t bestStart = 0;

		for (size_t i = 0; i < tasks.size(); ++i)
		{
			const CookTask& task = tasks[i];
			if (scheduledIds.find(task.m_id) != scheduledIds.end())
			{
				continue;
			}
			if (!CookSchedule::dependenciesDone(task, done))
			{
				continue;
			}

			int32_t start = CookSchedule::dependenciesEnd(task, done);
			if (!task.m_resource.empty())
			{
				start = std::max(start, resourceAvailable[task.m_resource]);
			}
			if (task.m_active)
			{
				start = std::max(start, userAvailable);
			}

			bool choose = false;
			if (bestIndex < 0 || start < bestStart)
			{
				choose = true;
			}
			else if (start == bestStart)
			{
				const CookTask& currentBest = tasks[static_cast<size_t>(bestIndex)];
				if (!task.m_active && currentBest.m_active)
				{
					choose = true;
				}
				else if (task.m_active == currentBest.m_active)
				{
					int32_t taskWaitScore = waitScore(task.m_id);
					int32_t bestWaitScore = waitScore(currentBest.m_id);
					if (taskWaitScore > bestWaitScore)
					{
						choose = true;
					}
					else if (taskWaitScore == bestWaitScore && criticalScore(task.m_id) > criticalScore(currentBest.m_id))
					{
						choose = true;
					}
				}
			}

			if (choose)
			{
				bestIndex = static_cast<int32_t>(i);
				bestStart = start;
			}
		}

		if (bestIndex < 0)
		{
			LOGERROR("CookSchedule buildPlan stopped, unresolved dependencies totalTasks=%d scheduledTasks=%d",
			         static_cast<int32_t>(tasks.size()),
			         static_cast<int32_t>(scheduledIds.size()));
			break;
		}

		const CookTask& task = tasks[static_cast<size_t>(bestIndex)];
		ScheduledTask scheduled;
		static_cast<CookTask&>(scheduled) = task;
		scheduled.m_startSeconds = bestStart;
		scheduled.m_endSeconds = bestStart + task.m_durationSeconds;
		plan.m_timeline.push_back(scheduled);
		done[scheduled.m_id] = scheduled;
		scheduledIds.insert(scheduled.m_id);
		LOGDEBUG("CookSchedule task scheduled taskId=%s recipeId=%s startSeconds=%d endSeconds=%d active=%d resource=%s",
		         scheduled.m_id.c_str(),
		         scheduled.m_recipeId.c_str(),
		         scheduled.m_startSeconds,
		         scheduled.m_endSeconds,
		         scheduled.m_active ? 1 : 0,
		         scheduled.m_resource.c_str());

		if (scheduled.m_active)
		{
			userAvailable = scheduled.m_endSeconds;
			plan.m_activeSeconds += scheduled.m_durationSeconds;
		}
		if (!scheduled.m_resource.empty())
		{
			resourceAvailable[scheduled.m_resource] = scheduled.m_endSeconds;
		}
		plan.m_totalSeconds = std::max(plan.m_totalSeconds, scheduled.m_endSeconds);
	}

	std::sort(plan.m_timeline.begin(), plan.m_timeline.end(), [](const ScheduledTask& lhs, const ScheduledTask& rhs) {
		if (lhs.m_startSeconds != rhs.m_startSeconds)
		{
			return lhs.m_startSeconds < rhs.m_startSeconds;
		}
		return lhs.m_active < rhs.m_active;
	});

	std::vector<std::pair<int32_t, int32_t>> activeIntervals;
	for (size_t i = 0; i < plan.m_timeline.size(); ++i)
	{
		if (plan.m_timeline[i].m_active)
		{
			activeIntervals.push_back(std::make_pair(plan.m_timeline[i].m_startSeconds, plan.m_timeline[i].m_endSeconds));
		}
	}
	std::sort(activeIntervals.begin(), activeIntervals.end());

	int32_t cursor = 0;
	for (size_t i = 0; i < activeIntervals.size(); ++i)
	{
		int32_t gapStart = cursor;
		int32_t gapEnd = activeIntervals[i].first;
		if (gapEnd > gapStart)
		{
			CookSchedule::addGapToPlan(plan, plan.m_timeline, gapStart, gapEnd);
		}
		cursor = std::max(cursor, activeIntervals[i].second);
	}

	if (plan.m_totalSeconds > cursor)
	{
		CookSchedule::addGapToPlan(plan, plan.m_timeline, cursor, plan.m_totalSeconds);
	}

	LOGDEBUG("CookSchedule buildPlan finish selectedRecipes=%d timelineCount=%d totalSeconds=%d activeSeconds=%d freeSeconds=%d edgeSeconds=%d",
	         static_cast<int32_t>(plan.m_recipes.size()),
	         static_cast<int32_t>(plan.m_timeline.size()),
	         plan.m_totalSeconds,
	         plan.m_activeSeconds,
	         plan.m_freeSeconds,
	         plan.m_edgeSeconds);
}

PlanResult CookSchedule::buildPlan(const std::vector<std::string>& recipeIds)
{
	LOGDEBUG("CookSchedule buildPlan begin recipeCount=%d", static_cast<int32_t>(recipeIds.size()));
	PlanResult plan;
	std::vector<CookTask> tasks;
	std::set<std::string> selected;

	for (size_t i = 0; i < recipeIds.size(); ++i)
	{
		if (selected.find(recipeIds[i]) != selected.end())
		{
			LOGDEBUG("CookSchedule skip duplicated recipeId=%s", recipeIds[i].c_str());
			continue;
		}
		selected.insert(recipeIds[i]);
		const Recipe* recipe = CookCatalog::findRecipe(recipeIds[i]);
		if (recipe == nullptr)
		{
			LOGWARNING("CookSchedule skip unknown recipeId=%s", recipeIds[i].c_str());
			continue;
		}
		LOGDEBUG("CookSchedule select recipeId=%s title=%s taskCount=%d",
		         recipe->m_id.c_str(),
		         recipe->m_title.c_str(),
		         static_cast<int32_t>(recipe->m_tasks.size()));
		plan.m_recipes.push_back(recipe);
		for (size_t j = 0; j < recipe->m_tasks.size(); ++j)
		{
			tasks.push_back(recipe->m_tasks[j]);
		}
	}

	CookSchedule::schedulePreparedPlan(plan, tasks);
	return plan;
}

PlanResult CookSchedule::buildPlanFromRecipes(const std::vector<Recipe>& recipes)
{
	LOGDEBUG("CookSchedule buildPlanFromRecipes begin recipeCount=%d", static_cast<int32_t>(recipes.size()));
	PlanResult plan;
	std::vector<CookTask> tasks;
	std::set<std::string> selected;

	for (size_t i = 0; i < recipes.size(); ++i)
	{
		if (recipes[i].m_id.empty() || selected.find(recipes[i].m_id) != selected.end())
		{
			continue;
		}
		selected.insert(recipes[i].m_id);
		plan.m_recipeCopies.push_back(recipes[i]);
	}
	plan.m_recipes.reserve(plan.m_recipeCopies.size());
	for (size_t i = 0; i < plan.m_recipeCopies.size(); ++i)
	{
		const Recipe& recipe = plan.m_recipeCopies[i];
		plan.m_recipes.push_back(&plan.m_recipeCopies[i]);
		for (size_t j = 0; j < recipe.m_tasks.size(); ++j)
		{
			tasks.push_back(recipe.m_tasks[j]);
		}
		if (recipe.m_personalizationApplied && !recipe.m_personalizationSummary.empty())
		{
			plan.m_warnings.push_back(recipe.m_personalizationSummary);
		}
	}

	CookSchedule::schedulePreparedPlan(plan, tasks);
	return plan;
}