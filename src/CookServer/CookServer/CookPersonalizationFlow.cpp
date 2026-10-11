#include "CookPersonalizationFlow.h"
#include "CookRecipeConfig.h"
#include "CookHelper.h"
#include <algorithm>
#include <functional>
#include <map>
#include <set>

bool CookPersonalizationFlow::Build(const Recipe& base, const PersonalizationConfig& config, Recipe& result, std::string& message)
{
    if (base.m_tasks.size() + config.m_addedTasks.size() > 256 || config.m_taskOrder.size() != base.m_tasks.size() + config.m_addedTasks.size())
    { message = "Step order must contain all steps, at most 256"; return false; }
    std::map<std::string, CookTask> tasks;
    std::set<std::string> added;
    std::vector<std::string> baseOrder;
    for (size_t i = 0; i < base.m_tasks.size(); ++i)
    {
        tasks[base.m_tasks[i].m_id] = base.m_tasks[i];
        baseOrder.push_back(base.m_tasks[i].m_id);
    }
    for (size_t i = 0; i < config.m_addedTasks.size(); ++i)
    {
        CookTask task = config.m_addedTasks[i];
        task.m_shortId = CookHelper::taskShortId(task);
        task.m_id = CookHelper::fullDependencyId(base.m_id, task.m_shortId);
        task.m_recipeId = base.m_id;
        if (task.m_shortId.empty() || tasks.count(task.m_id) != 0 || task.m_durationSeconds <= 0)
        { message = "Added step has a duplicate ID or invalid duration"; return false; }
        task.m_voiceText = task.m_title;
        tasks[task.m_id] = task;
        added.insert(task.m_id);
    }
    Recipe submitted = base;
    submitted.m_tasks.clear();
    for (std::map<std::string, CookTask>::const_iterator it = tasks.begin(); it != tasks.end(); ++it)
    {
        submitted.m_tasks.push_back(it->second);
    }
    if (!CookRecipeConfig::validateTaskGraph(submitted, &message)) return false;
    std::vector<std::string> order;
    std::set<std::string> used;
    size_t nextBase = 0;
    for (size_t i = 0; i < config.m_taskOrder.size(); ++i)
    {
        const std::string id = CookHelper::fullDependencyId(base.m_id, config.m_taskOrder[i]);
        if (tasks.count(id) == 0 || !used.insert(id).second)
        { message = "Step order contains an unknown or duplicate ID"; return false; }
        if (added.count(id) == 0)
        {
            if (nextBase >= baseOrder.size() || baseOrder[nextBase++] != id)
            { message = "Original step order cannot be changed"; return false; }
        }
        order.push_back(id);
    }
    // 连续新增步骤作为一个插入段，继承后一原步骤的先决条件。
    // 前一原步骤也须已完成，避免仅改变显示而与实际操作并行。
    std::string previousBase;
    for (size_t i = 0; i < order.size();)
    {
        if (added.count(order[i]) == 0) { previousBase = order[i++]; continue; }
        const size_t begin = i;
        while (i < order.size() && added.count(order[i]) != 0) ++i;
        std::vector<std::string> dependencies;
        if (i < order.size()) dependencies = tasks[order[i]].m_dependencies;
        if (!previousBase.empty() && std::find(dependencies.begin(), dependencies.end(), previousBase) == dependencies.end()) dependencies.push_back(previousBase);
        for (size_t j = begin; j < i; ++j)
        {
            tasks[order[j]].m_dependencies = dependencies;
            dependencies.assign(1, order[j]);
        }
        if (i < order.size()) tasks[order[i]].m_dependencies = dependencies;
    }
    result = base;
    result.m_tasks.clear();
    for (size_t i = 0; i < order.size(); ++i) result.m_tasks.push_back(tasks[order[i]]);
    if (!CookRecipeConfig::validateTaskGraph(result, &message)) return false;
    std::set<std::string> disabled;
    for (std::set<std::string>::const_iterator it = config.m_disabledTaskIds.begin(); it != config.m_disabledTaskIds.end(); ++it)
    {
        const std::string id = CookHelper::fullDependencyId(base.m_id, *it);
        if (tasks.count(id) == 0) { message = "Disabled step does not exist"; return false; }
        disabled.insert(id);
    }
    std::map<std::string, std::vector<std::string>> resolved;
    std::function<std::vector<std::string>(const std::string&)> expand = [&](const std::string& id) -> std::vector<std::string>
    {
        if (disabled.count(id) == 0) return std::vector<std::string>(1, id);
        if (resolved.count(id) != 0) return resolved[id];
        std::set<std::string> flattened;
        const std::vector<std::string>& dependencies = tasks[id].m_dependencies;
        for (size_t i = 0; i < dependencies.size(); ++i)
        {
            const std::vector<std::string> parents = expand(dependencies[i]);
            flattened.insert(parents.begin(), parents.end());
        }
        resolved[id] = std::vector<std::string>(flattened.begin(), flattened.end());
        return resolved[id];
    };
    result.m_tasks.clear();
    for (size_t i = 0; i < order.size(); ++i)
    {
        if (disabled.count(order[i]) != 0) continue;
        CookTask task = tasks[order[i]];
        std::set<std::string> dependencies;
        for (size_t j = 0; j < task.m_dependencies.size(); ++j)
        {
            const std::vector<std::string> expanded = expand(task.m_dependencies[j]);
            dependencies.insert(expanded.begin(), expanded.end());
        }
        task.m_dependencies.assign(dependencies.begin(), dependencies.end());
        for (std::map<std::string, int32_t>::const_iterator it = config.m_durationOverrides.begin(); it != config.m_durationOverrides.end(); ++it)
        {
            if (it->second >= 0 && CookHelper::taskIdMatches(task, it->first)) task.m_durationSeconds = it->second;
        }
        result.m_tasks.push_back(task);
    }
    return result.m_tasks.empty() || CookRecipeConfig::validateTaskGraph(result, &message);
}