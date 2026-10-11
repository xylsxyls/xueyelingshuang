#include "AccountStore.h"
#include "CookCatalog.h"
#include "CookHelper.h"
#include "CookServerHelper.h"
#include <algorithm>

bool AccountStore::isRecipeFree(const Recipe& recipe)
{
    return recipe.m_priceType == "free" || (recipe.m_priceAmount <= 0 && recipe.m_priceCoins <= 0);
}

bool AccountStore::isRecipeInMenu(const Recipe& recipe, const UserAccount& account)
{
    return std::find(account.m_menuRecipeOrder.begin(), account.m_menuRecipeOrder.end(), recipe.m_id) != account.m_menuRecipeOrder.end();
}

void AccountStore::refreshMenuNoLock(UserAccount& account) const
{
    if (!account.m_menuInitialized)
    {
        // 旧的拥有集合包含作者自己的作品；创作并不等于主动加入菜单。
        for (std::set<std::string>::const_iterator it = account.m_ownedRecipeIds.begin(); it != account.m_ownedRecipeIds.end(); ++it)
        {
            const Recipe* recipe = findBaseRecipeNoLock(*it);
            if (recipe != nullptr && recipe->m_authorUserId != account.m_userId &&
                (isRecipeFree(*recipe) || account.m_purchasedRecipeIds.count(*it) != 0))
            {
                account.m_menuRecipeOrder.push_back(*it);
            }
        }
        account.m_menuInitialized = true;
    }
    const std::vector<Recipe>& defaults = CookCatalog::recipeCatalog();
    for (size_t i = 0; i < defaults.size(); ++i)
    {
        if (defaults[i].m_systemRecipe && isRecipeFree(defaults[i]) &&
            account.m_removedMenuRecipeIds.count(defaults[i].m_id) == 0 && !isRecipeInMenu(defaults[i], account))
        {
            account.m_menuRecipeOrder.push_back(defaults[i].m_id);
        }
    }
    for (std::map<std::string, Recipe>::const_iterator it = m_customRecipes.begin(); it != m_customRecipes.end(); ++it)
    {
        const Recipe& recipe = it->second;
        if (recipe.m_systemRecipe && recipe.m_status == "published" && isRecipeFree(recipe) &&
            account.m_removedMenuRecipeIds.count(recipe.m_id) == 0 && !isRecipeInMenu(recipe, account))
        {
            account.m_menuRecipeOrder.push_back(recipe.m_id);
        }
    }
    for (std::set<std::string>::const_iterator it = account.m_purchasedRecipeIds.begin(); it != account.m_purchasedRecipeIds.end(); ++it)
    {
        if (std::find(account.m_menuRecipeOrder.begin(), account.m_menuRecipeOrder.end(), *it) == account.m_menuRecipeOrder.end())
        {
            account.m_menuRecipeOrder.insert(account.m_menuRecipeOrder.begin(), *it);
        }
    }
}

UserAccount AccountStore::joinRecipe(const std::string& userId, const std::string& recipeId,
    const std::string& idempotencyKey, bool* ok, bool* alreadyOwned, std::string* message, Recipe* recipe)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& account = ensureNoLock(userId);
    const Recipe* base = findBaseRecipeNoLock(normalizeRecipeIdNoLock(recipeId));
    if (ok) *ok = false;
    if (alreadyOwned) *alreadyOwned = false;
    if (base == nullptr || base->m_status != "published")
    {
        if (message) *message = "Recipe is not published";
        return account;
    }
    const bool inMenu = isRecipeInMenu(*base, account);
    // 加入只改变菜单归属，付费菜谱的权限必须由购买接口授予。
    if (!inMenu)
    {
        account.m_menuRecipeOrder.insert(account.m_menuRecipeOrder.begin(), base->m_id);
        account.m_removedMenuRecipeIds.erase(base->m_id);
        saveNoLock();
    }
    if (ok) *ok = true;
    if (alreadyOwned) *alreadyOwned = inMenu;
    if (message) *message = inMenu ? "Already in menu" : "Added to menu";
    if (recipe)
    {
        *recipe = *base;
        decorateRecipeMediaNoLock(*recipe);
        if (isRecipeOwned(*base, account)) applyPersonalizationNoLock(account.m_userId, *recipe);
    }
    return account;
}

bool AccountStore::removeMenuRecipe(const std::string& userId, const std::string& recipeId, std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& account = ensureNoLock(userId);
    const std::string id = normalizeRecipeIdNoLock(recipeId);
    if (account.m_purchasedRecipeIds.count(id) != 0)
    {
        message = "Purchased recipes cannot be removed";
        return false;
    }
    if (findBaseRecipeNoLock(id) == nullptr)
    {
        message = "Recipe not found";
        return false;
    }
    account.m_menuRecipeOrder.erase(std::remove(account.m_menuRecipeOrder.begin(), account.m_menuRecipeOrder.end(), id), account.m_menuRecipeOrder.end());
    account.m_removedMenuRecipeIds.insert(id);
    saveNoLock();
    message = "Removed from menu";
    return true;
}

void AccountStore::touchMenuRecipe(const std::string& userId, const std::string& recipeId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    UserAccount& account = ensureNoLock(userId);
    std::vector<std::string>::iterator it = std::find(account.m_menuRecipeOrder.begin(), account.m_menuRecipeOrder.end(), recipeId);
    if (it == account.m_menuRecipeOrder.end() || it == account.m_menuRecipeOrder.begin()) return;
    account.m_menuRecipeOrder.erase(it);
    account.m_menuRecipeOrder.insert(account.m_menuRecipeOrder.begin(), recipeId);
    saveNoLock();
}

void AccountStore::decorateRecipeMediaNoLock(Recipe& recipe)
{
    syncResourceVideosNoLock(false);
    const VideoInfo* chosen = nullptr;
    for (std::map<std::string, VideoInfo>::const_iterator it = m_videos.begin(); it != m_videos.end(); ++it)
    {
        const VideoInfo& video = it->second;
        if (video.m_status != "published" || video.m_mediaType != "video" ||
            std::find(video.m_recipeIds.begin(), video.m_recipeIds.end(), recipe.m_id) == video.m_recipeIds.end()) continue;
        const bool original = video.m_ownerUserId == recipe.m_authorUserId || (recipe.m_systemRecipe && video.m_ownerUserId == "qmzdc");
        const bool chosenOriginal = chosen != nullptr && (chosen->m_ownerUserId == recipe.m_authorUserId || (recipe.m_systemRecipe && chosen->m_ownerUserId == "qmzdc"));
        if (chosen == nullptr || (original && !chosenOriginal) || (original == chosenOriginal && video.m_createdAtSeconds > chosen->m_createdAtSeconds)) chosen = &video;
    }
    recipe.m_videoId = chosen == nullptr ? "" : chosen->m_id;
}