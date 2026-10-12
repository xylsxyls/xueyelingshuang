#include "AccountStore.h"
#include "CookCatalog.h"
#include "CookHelper.h"
#include "CookRecipePresentation.h"
#include "CSystem/CSystemAPI.h"

bool AccountStore::saveCustomRecipe(
    const std::string &userId, const Recipe &sourceRecipe, bool published, Recipe &recipe, std::string &message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount &account = ensureNoLock(userId);
    if (sourceRecipe.m_id.empty() || sourceRecipe.m_id.size() > 128 || sourceRecipe.m_tasks.empty() ||
        !CookRecipePresentation::Validate(sourceRecipe, message))
    {
        message = "创建标识、菜谱名称或步骤无效";
        return false;
    }
    for (size_t i = 0; i < sourceRecipe.m_id.size(); ++i)
    {
        const char c = sourceRecipe.m_id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' ||
                c == '.'))
        {
            message = "客户端创建标识格式无效";
            return false;
        }
    }
    if (CookCatalog::findRecipe(sourceRecipe.m_id) != nullptr)
    {
        message = "内置菜谱请从创作编辑入口修改";
        return false;
    }
    std::string id = sourceRecipe.m_id;
    const Recipe *current = findBaseRecipeNoLock(id);
    if (current != nullptr && current->m_authorUserId != userId)
    {
        message = "不能修改其他作者的菜谱";
        return false;
    }
    const std::string alias = userId + "|" + sourceRecipe.m_id;
    if (m_creationAliases.count(alias) != 0)
    {
        id = m_creationAliases[alias];
    }
    current = findCreationRecipeNoLock(userId, id);
    if (current == nullptr)
    {
        id = "recipe_" + CSystem::uuid(0);
        if (id.size() != 39)
        {
            message = "无法生成菜谱标识";
            return false;
        }
    }
    else if (!sourceRecipe.m_currentRevisionId.empty() &&
             sourceRecipe.m_currentRevisionId != sourceRecipe.m_id + "_r1" &&
             sourceRecipe.m_currentRevisionId != current->m_currentRevisionId)
    {
        message = "菜谱已更新，请读取最新版本后再提交";
        return false;
    }
    recipe = sourceRecipe;
    recipe.m_id = id;
    recipe.m_creationId =
        current != nullptr && !current->m_creationId.empty() ? current->m_creationId : "creation_" + CSystem::uuid(0);
    recipe.m_authorUserId = userId;
    recipe.m_author = account.m_nickname;
    recipe.m_systemRecipe = userId == "qmzdc";
    recipe.m_customRecipe = true;
    recipe.m_personalizationApplied = false;
    recipe.m_personalizationSummary.clear();
    if (recipe.m_priceType.empty())
    {
        recipe.m_priceType = recipe.m_priceAmount > 0 || recipe.m_priceCoins > 0 ? "coin" : "free";
    }
    if (recipe.m_priceAmount == 0 && recipe.m_priceCoins > 0 && recipe.m_priceType == "coin")
    {
        recipe.m_priceAmount = recipe.m_priceCoins;
    }
    for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
    {
        CookTask &task = recipe.m_tasks[i];
        task.m_shortId = CookHelper::taskShortId(task);
        task.m_id = id + "." + task.m_shortId;
        task.m_recipeId = id;
        for (size_t j = 0; j < task.m_dependencies.size(); ++j)
        {
            const std::string shortId = CookHelper::shortDependencyId(sourceRecipe.m_id, task.m_dependencies[j]);
            task.m_dependencies[j] = CookHelper::fullDependencyId(id, shortId);
        }
    }
    return submitCreationNoLock(userId, recipe, sourceRecipe.m_id, published, message);
}