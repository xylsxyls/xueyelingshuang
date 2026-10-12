#include "CookCreationFlow.h"
#include "CookHelper.h"
#include "CookPersonalizationFlow.h"
#include <algorithm>

bool CookCreationFlow::rebasePersonalization(const Recipe& base, const Recipe& edited,
    const PersonalizationConfig& source, PersonalizationConfig& result, std::string& message)
{
    result = source;
    std::map<std::string, CookTask> original;
    std::set<std::string> kept;
    for (size_t i = 0; i < base.m_tasks.size(); ++i)
    {
        original[base.m_tasks[i].m_id] = base.m_tasks[i];
    }
    for (size_t i = 0; i < edited.m_tasks.size(); ++i)
    {
        kept.insert(edited.m_tasks[i].m_id);
    }
    std::set<std::string> added;
    for (size_t i = 0; i < source.m_addedTasks.size(); ++i)
    {
        const std::string id = CookHelper::fullDependencyId(base.m_id, CookHelper::taskShortId(source.m_addedTasks[i]));
        if (kept.count(id) != 0 || !added.insert(id).second)
        {
            message = "New step ID conflicts with a saved personal step";
            return false;
        }
    }
    std::set<std::string> available = kept;
    available.insert(added.begin(), added.end());
    for (std::map<std::string, int32_t>::iterator it = result.m_durationOverrides.begin(); it != result.m_durationOverrides.end();)
    {
        if (available.count(CookHelper::fullDependencyId(base.m_id, it->first)) == 0)
        {
            result.m_durationOverrides.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    for (std::set<std::string>::iterator it = result.m_disabledTaskIds.begin(); it != result.m_disabledTaskIds.end();)
    {
        if (available.count(CookHelper::fullDependencyId(base.m_id, *it)) == 0)
        {
            result.m_disabledTaskIds.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    std::map<std::string, std::set<std::string>> cache;
    std::set<std::string> visiting;
    for (size_t i = 0; i < result.m_addedTasks.size(); ++i)
    {
        CookTask& task = result.m_addedTasks[i];
        std::set<std::string> dependencies;
        for (size_t j = 0; j < task.m_dependencies.size(); ++j)
        {
            const std::string id = CookHelper::fullDependencyId(base.m_id, task.m_dependencies[j]);
            if (available.count(id) != 0)
            {
                dependencies.insert(id);
            }
            else if (!expandRemoved(id, original, kept, cache, visiting, dependencies))
            {
                message = "Cannot retain a personal step dependency";
                return false;
            }
        }
        task.m_dependencies.assign(dependencies.begin(), dependencies.end());
    }
    std::map<std::string, std::vector<std::string>> before;
    std::set<std::string> used;
    std::string anchor;
    for (size_t i = source.m_taskOrder.size(); i > 0; --i)
    {
        const std::string id = CookHelper::fullDependencyId(base.m_id, source.m_taskOrder[i - 1]);
        if (kept.count(id) != 0)
        {
            anchor = id;
        }
        else if (added.count(id) != 0 && used.insert(id).second)
        {
            before[anchor].insert(before[anchor].begin(), id);
        }
    }
    for (size_t i = 0; i < result.m_addedTasks.size(); ++i)
    {
        const std::string id = CookHelper::fullDependencyId(base.m_id, CookHelper::taskShortId(result.m_addedTasks[i]));
        if (used.insert(id).second)
        {
            before[""].push_back(id);
        }
    }
    result.m_taskOrder.clear();
    for (size_t i = 0; i < edited.m_tasks.size(); ++i)
    {
        const std::string& id = edited.m_tasks[i].m_id;
        result.m_taskOrder.insert(result.m_taskOrder.end(), before[id].begin(), before[id].end());
        result.m_taskOrder.push_back(id);
    }
    result.m_taskOrder.insert(result.m_taskOrder.end(), before[""].begin(), before[""].end());
    Recipe check;
    return CookPersonalizationFlow::Build(edited, result, check, message);
}