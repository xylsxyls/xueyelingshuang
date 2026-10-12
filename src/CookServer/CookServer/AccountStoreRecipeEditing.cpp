#include "AccountStore.h"
#include "CookCreationFlow.h"
#include "CSystem/CSystemAPI.h"

bool AccountStore::getCreationRecipe(const std::string& userId, const std::string& recipeId, Recipe& recipe, std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    const Recipe* base = findCreationRecipeNoLock(userId, recipeId);
    if (base == nullptr || base->m_authorUserId != userId)
    {
        message = "Only the author can edit this recipe";
        return false;
    }
    recipe = *base;
    decorateRecipeMediaNoLock(recipe);
    decorateCreationNoLock(userId, recipe);
    message = "ok";
    return true;
}

bool AccountStore::saveCreationSteps(const std::string& userId, const std::string& recipeId, const std::string& revision,
    const std::vector<CookTask>& steps, const std::string& priceType, int32_t priceAmount, int32_t editorVersion,
    Recipe& recipe, std::string& message, const std::vector<std::string>& mediaIds, bool mediaSpecified,
    const std::string& coverImageId, bool coverSpecified)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    const Recipe* base = findCreationRecipeNoLock(userId, recipeId);
    if (base == nullptr || base->m_authorUserId != userId)
    {
        message = "Only the author can edit this recipe";
        return false;
    }
    if (revision.empty() || revision != base->m_currentRevisionId)
    {
        message = "Recipe changed; reload before saving";
        return false;
    }
    Recipe edited;
    bool structureChanged = false;
    if (!CookCreationFlow::buildSteps(*base, steps, editorVersion == 2, edited, structureChanged, message))
    {
        return false;
    }
    if (!priceType.empty())
    {
        if ((priceType != "free" && priceType != "coin" && priceType != "yuanbao") ||
            (priceType == "free" ? priceAmount != 0 : priceAmount <= 0))
        {
            message = "Invalid recipe price type or amount";
            return false;
        }
        edited.m_priceType = priceType;
        edited.m_priceAmount = priceAmount;
        edited.m_priceCoins = priceType == "coin" ? priceAmount : 0;
        edited.m_defaultOwned = priceType == "free";
    }
    if (mediaSpecified)
    {
        edited.m_mediaIds = mediaIds;
    }
    else if (edited.m_mediaIds.empty())
    {
        decorateRecipeMediaNoLock(edited);
        if (!edited.m_videoId.empty())
        {
            edited.m_mediaIds.push_back(edited.m_videoId);
        }
    }
    if (coverSpecified)
    {
        edited.m_coverImageId = coverImageId;
    }
    if (edited.m_creationId.empty())
    {
        edited.m_creationId = "creation_" + CSystem::uuid(0);
    }
    if (!submitCreationNoLock(userId, edited, "", true, message))
    {
        return false;
    }
    recipe = edited;
    return true;
}