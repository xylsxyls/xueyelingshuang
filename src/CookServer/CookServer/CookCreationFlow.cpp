#include "CookCreationFlow.h"
#include "CookRecipeConfig.h"
#include "CookRecipePresentation.h"
#include "Config.h"

bool CookCreationFlow::validNewId(const std::string& recipeId, const std::string& id)
{
    const std::string prefix = recipeId + ".added_";
    if (id.find(prefix) != 0 || id.size() <= prefix.size() || id.size() > recipeId.size() + 129)
    {
        return false;
    }
    for (size_t i = prefix.size(); i < id.size(); ++i)
    {
        const char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            return false;
        }
    }
    return true;
}

void CookCreationFlow::applyMode(CookTask& task, bool active, bool created)
{
    if (created || task.m_active != active)
    {
        task.m_manualSkippable = active;
        task.m_canPause = active && (created ? task.m_safetyLevel == "normal" : task.m_canPause);
        task.m_continuesDuringPause = !active;
        if (!created)
        {
            task.m_canLeaveKitchen = false;
            task.m_backgroundWaitMode = active ? "" : "watch";
        }
    }
    task.m_active = active;
    if (active)
    {
        task.m_canLeaveKitchen = false;
        task.m_backgroundWaitMode.clear();
    }
    else
    {
        const bool free = task.m_backgroundWaitMode == "free" && task.m_canLeaveKitchen && task.m_safetyLevel == "normal";
        task.m_canLeaveKitchen = free;
        task.m_backgroundWaitMode = free ? "free" : "watch";
    }
}

bool CookCreationFlow::expandRemoved(const std::string& id, const std::map<std::string, CookTask>& original,
    const std::set<std::string>& kept, std::map<std::string, std::set<std::string>>& cache,
    std::set<std::string>& visiting, std::set<std::string>& result)
{
    if (kept.count(id) != 0)
    {
        result.insert(id);
        return true;
    }
    if (cache.count(id) != 0)
    {
        result.insert(cache[id].begin(), cache[id].end());
        return true;
    }
    const std::map<std::string, CookTask>::const_iterator found = original.find(id);
    if (found == original.end() || !visiting.insert(id).second)
    {
        return false;
    }
    std::set<std::string> parents;
    for (size_t i = 0; i < found->second.m_dependencies.size(); ++i)
    {
        if (!expandRemoved(found->second.m_dependencies[i], original, kept, cache, visiting, parents))
        {
            return false;
        }
    }
    visiting.erase(id);
    cache[id] = parents;
    result.insert(parents.begin(), parents.end());
    return true;
}

bool CookCreationFlow::buildSteps(const Recipe& base, const std::vector<CookTask>& input, bool replaceStructure,
    Recipe& result, bool& structureChanged, std::string& message)
{
    if (input.empty() || input.size() > 256 || (!replaceStructure && input.size() != base.m_tasks.size()))
    {
        message = "A complete list of 1 to 256 steps is required";
        return false;
    }
    if (!CookRecipeConfig::validateTaskGraph(base, &message))
    {
        return false;
    }
    std::map<std::string, CookTask> original;
    for (size_t i = 0; i < base.m_tasks.size(); ++i)
    {
        original[base.m_tasks[i].m_id] = base.m_tasks[i];
    }
    result = base;
    result.m_tasks.clear();
    std::set<std::string> kept;
    std::map<std::string, size_t> position;
    structureChanged = input.size() != base.m_tasks.size();
    for (size_t i = 0; i < input.size(); ++i)
    {
        const CookTask& value = input[i];
        const std::map<std::string, CookTask>::const_iterator old = original.find(value.m_id);
        const bool created = old == original.end();
        if (!kept.insert(value.m_id).second || (created && (!replaceStructure || !validNewId(base.m_id, value.m_id))) ||
            !CookRecipePresentation::ValidateTitle(value.m_title) || value.m_detail.size() > static_cast<size_t>(g_config.m_creationStepMaxDetailBytes) ||
            value.m_durationSeconds < 0 || value.m_durationSeconds > g_config.m_creationStepMaxSeconds)
        {
            message = "Invalid step ID, title, description or duration";
            return false;
        }
        CookTask task = created ? CookTask() : old->second;
        if (created)
        {
            if ((value.m_safetyLevel != "normal" && value.m_safetyLevel != "attention" && value.m_safetyLevel != "danger") ||
                (value.m_backgroundWaitMode != "watch" && value.m_backgroundWaitMode != "free"))
            {
                message = "Invalid new step safety or waiting mode";
                return false;
            }
            task.m_id = value.m_id;
            task.m_shortId = value.m_id.substr(base.m_id.size() + 1);
            task.m_recipeId = base.m_id;
            task.m_safetyLevel = value.m_safetyLevel;
            task.m_backgroundWaitMode = value.m_backgroundWaitMode;
            task.m_canLeaveKitchen = value.m_canLeaveKitchen;
        }
        task.m_title = value.m_title;
        task.m_voiceText = value.m_title;
        task.m_detail = value.m_detail;
        task.m_durationSeconds = value.m_durationSeconds;
        if (replaceStructure)
        {
            applyMode(task, value.m_active, created);
            structureChanged = structureChanged || i >= base.m_tasks.size() || base.m_tasks[i].m_id != task.m_id;
        }
        result.m_tasks.push_back(task);
        position[task.m_id] = i;
    }
    if (!replaceStructure)
    {
        // 旧协议只改原步骤内容，保持原有顺序。
        std::map<std::string, CookTask> byId;
        for (size_t i = 0; i < result.m_tasks.size(); ++i)
        {
            byId[result.m_tasks[i].m_id] = result.m_tasks[i];
        }
        result.m_tasks.clear();
        for (size_t i = 0; i < base.m_tasks.size(); ++i)
        {
            result.m_tasks.push_back(byId[base.m_tasks[i].m_id]);
        }
        structureChanged = false;
    }
    else if (structureChanged)
    {
        std::map<std::string, std::set<std::string>> cache;
        std::set<std::string> visiting;
        std::string previousForeground;
        for (size_t i = 0; i < result.m_tasks.size(); ++i)
        {
            CookTask& task = result.m_tasks[i];
            std::set<std::string> dependencies;
            for (size_t j = 0; j < task.m_dependencies.size(); ++j)
            {
                if (!expandRemoved(task.m_dependencies[j], original, kept, cache, visiting, dependencies))
                {
                    message = "Cannot resolve removed step dependencies";
                    return false;
                }
            }
            task.m_dependencies.clear();
            for (std::set<std::string>::const_iterator it = dependencies.begin(); it != dependencies.end(); ++it)
            {
                if (position[*it] < i)
                {
                    task.m_dependencies.push_back(*it);
                }
            }
            if (!previousForeground.empty() && dependencies.count(previousForeground) == 0)
            {
                task.m_dependencies.push_back(previousForeground);
            }
            if (task.m_active)
            {
                previousForeground = task.m_id;
            }
        }
    }
    return CookRecipeConfig::validateTaskGraph(result, &message);
}