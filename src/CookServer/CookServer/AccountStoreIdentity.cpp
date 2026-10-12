#include "AccountStore.h"
#include "Config.h"
#include "CookCatalog.h"
#include "CookServerHelper.h"
#include "LogManager/LogManagerAPI.h"

bool AccountStore::migrateSystemIdNoLock(bool& changed, std::string& message)
{
    const std::string oldId = "system";
    const std::string newId = "qmzdc";
    if (m_accounts.count(oldId) != 0 && m_accounts.count(newId) != 0)
    {
        message = "Both legacy and new system identities exist; migration refused";
        return false;
    }
    for (std::map<std::string, PersonalizationConfig>::const_iterator it = m_personalizations.begin(); it != m_personalizations.end(); ++it)
    {
        if (it->second.m_userId == oldId && m_personalizations.count(newId + "|" + it->second.m_recipeId) != 0)
        {
            message = "System personalization keys conflict; migration refused";
            return false;
        }
    }
    if (m_accounts.count(oldId) != 0)
    {
        UserAccount account = m_accounts[oldId];
        account.m_userId = newId;
        m_accounts.erase(oldId);
        m_accounts[newId] = account;
        changed = true;
    }
    for (std::map<std::string, std::string>::iterator it = m_accountIndex.begin(); it != m_accountIndex.end(); ++it)
    {
        if (it->second == oldId)
        {
            it->second = newId;
            changed = true;
        }
    }
    const std::string oldAvatar = "/api/users/avatar?userId=system&key=";
    for (std::map<std::string, UserAccount>::iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
    {
        if (it->second.m_followingUserIds.erase(oldId) != 0)
        {
            it->second.m_followingUserIds.insert(newId);
            changed = true;
        }
        if (it->second.m_avatarUrl.compare(0, oldAvatar.size(), oldAvatar) == 0)
        {
            it->second.m_avatarUrl = "/api/users/avatar?userId=qmzdc&key=" + it->second.m_avatarUrl.substr(oldAvatar.size());
            changed = true;
        }
    }
    for (std::map<std::string, AuthSessionInfo>::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
    {
        if (it->second.m_userId == oldId)
        {
            it->second.m_userId = newId;
            changed = true;
        }
    }
    for (std::map<std::string, AdRewardInfo>::iterator it = m_adRewards.begin(); it != m_adRewards.end(); ++it)
    {
        if (it->second.m_userId == oldId)
        {
            it->second.m_userId = newId;
            changed = true;
        }
    }
    for (std::map<std::string, Recipe>::iterator it = m_customRecipes.begin(); it != m_customRecipes.end(); ++it)
    {
        if (it->second.m_authorUserId == oldId)
        {
            it->second.m_authorUserId = newId;
            it->second.m_author = CookServerHelper::textToJsonUtf8("全民做大厨");
            it->second.m_systemRecipe = true;
            changed = true;
        }
    }
    for (size_t i = 0; i < m_comments.size(); ++i)
    {
        if (m_comments[i].m_userId == oldId)
        {
            m_comments[i].m_userId = newId;
            m_comments[i].m_nickname = CookServerHelper::textToJsonUtf8("全民做大厨");
            changed = true;
        }
    }
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        if (m_messages[i].m_fromUserId == oldId)
        {
            m_messages[i].m_fromUserId = newId;
            changed = true;
        }
        if (m_messages[i].m_toUserId == oldId)
        {
            m_messages[i].m_toUserId = newId;
            changed = true;
        }
    }
    for (std::map<std::string, VideoInfo>::iterator it = m_videos.begin(); it != m_videos.end(); ++it)
    {
        if (it->second.m_ownerUserId == oldId)
        {
            it->second.m_ownerUserId = newId;
            changed = true;
        }
    }
    for (std::map<std::string, PersonalizationConfig>::iterator it = m_personalizations.begin(); it != m_personalizations.end();)
    {
        if (it->second.m_userId == oldId)
        {
            PersonalizationConfig config = it->second;
            config.m_userId = newId;
            m_personalizations.erase(it++);
            m_personalizations[newId + "|" + config.m_recipeId] = config;
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    for (std::map<std::string, PersonalizationConfig>::iterator it = m_sharedPersonalizations.begin(); it != m_sharedPersonalizations.end(); ++it)
    {
        if (it->second.m_userId == oldId)
        {
            it->second.m_userId = newId;
            changed = true;
        }
    }
    const std::string prefix = oldId + "|";
    for (std::set<std::string>::iterator it = m_idempotencyKeys.begin(); it != m_idempotencyKeys.end();)
    {
        if (it->compare(0, prefix.size(), prefix) == 0)
        {
            const std::string next = newId + "|" + it->substr(prefix.size());
            m_idempotencyKeys.erase(it++);
            m_idempotencyKeys.insert(next);
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    return true;
}

bool AccountStore::initialize(std::string& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    loadIfNeededNoLock();
    if (m_loadFailed)
    {
        message = "Cannot load account snapshot; refusing to replace existing data";
        return false;
    }
    const std::string systemId = "qmzdc";
    const std::string name = g_config.m_systemAccountName;
    std::map<std::string, std::string>::const_iterator occupied = m_accountIndex.find(name);
    if (occupied != m_accountIndex.end() && occupied->second != systemId && occupied->second != "system")
    {
        message = "System login name belongs to another user; migration refused";
        return false;
    }
    // 启动时保存旧状态；持久化失败完整恢复，并拒绝开放HTTP。
    const std::map<std::string, UserAccount> accounts = m_accounts;
    const std::map<std::string, std::string> index = m_accountIndex;
    const std::map<std::string, AuthSessionInfo> sessions = m_sessions;
    const std::map<std::string, AdRewardInfo> rewards = m_adRewards;
    const std::map<std::string, Recipe> recipes = m_customRecipes;
    const std::vector<CommentInfo> comments = m_comments;
    const std::vector<MessageInfo> messages = m_messages;
    const std::map<std::string, VideoInfo> videos = m_videos;
    const std::map<std::string, PersonalizationConfig> personalizations = m_personalizations;
    const std::map<std::string, PersonalizationConfig> shares = m_sharedPersonalizations;
    const std::set<std::string> idempotency = m_idempotencyKeys;
    const std::map<std::string, Recipe> drafts = m_creationDrafts;
    const std::map<std::string, std::string> bases = m_reviewBases;
    const std::map<std::string, CookReviewRevision> reviews = m_recipeReviews;
    const std::map<std::string, std::string> aliases = m_creationAliases;
    const std::map<std::string, std::map<std::string, int32_t>> usage = m_submissionUsage;
    // 冲突在任何修改前检查。
    const std::string sourceId = m_accounts.count("system") != 0 ? "system" : systemId;
    if (m_accounts.count(sourceId) != 0 && !m_accounts[sourceId].m_passwordHash.empty() && m_accounts[sourceId].m_account != name)
    {
        message = "Existing system credentials conflict with migration";
        return false;
    }
    bool changed = false;
    if (!migrateSystemIdNoLock(changed, message))
    {
        return false;
    }
    if (!migrateReviewIdentityNoLock(changed, message))
    {
        m_accounts = accounts; m_accountIndex = index; m_sessions = sessions; m_adRewards = rewards;
        m_customRecipes = recipes; m_comments = comments; m_messages = messages; m_videos = videos;
        m_personalizations = personalizations; m_sharedPersonalizations = shares; m_idempotencyKeys = idempotency;
        return false;
    }
    const bool created = m_accounts.count(systemId) == 0;
    UserAccount& account = m_accounts[systemId];
    if (sourceId == "system")
    {
        account.m_nickname = CookServerHelper::textToJsonUtf8("全民做大厨");
    }
    const bool bootstrap = created || account.m_passwordHash.empty();
    if (bootstrap)
    {
        account.m_userId = systemId;
        if (!account.m_account.empty() && account.m_account != name)
        {
            m_accountIndex.erase(account.m_account);
        }
        account.m_account = name;
        account.m_nickname = CookServerHelper::textToJsonUtf8("全民做大厨");
        account.m_passwordHash = g_config.m_systemPasswordHash;
        account.m_passwordSalt = g_config.m_systemPasswordSalt;
        if (created)
        {
            account.m_coins = g_config.m_accountDefaultCoins;
            account.m_yuanbao = g_config.m_accountDefaultYuanbao;
            account.m_createdAtSeconds = nowSecondsNoLock();
            account.m_lastLoginAtSeconds = account.m_createdAtSeconds;
        }
        refreshOwnedRecipesNoLock(account);
        refreshMenuNoLock(account);
        const std::vector<Recipe>& defaults = CookCatalog::recipeCatalog();
        for (size_t i = 0; i < defaults.size(); ++i)
        {
            if (defaults[i].m_authorUserId == systemId && !isRecipeInMenu(defaults[i], account))
            {
                account.m_menuRecipeOrder.push_back(defaults[i].m_id);
            }
        }
        changed = true;
    }
    m_accountIndex[name] = systemId;
    for (std::map<std::string, UserAccount>::iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
    {
        const std::string role = it->first == systemId ? "super_admin" : (it->second.m_account == "xylsxyls" ? "admin" : "user");
        if (it->second.m_role.empty() || (it->first == systemId && it->second.m_role != role) ||
            (it->second.m_account == "xylsxyls" && it->second.m_role != "admin"))
        {
            it->second.m_role = role;
            changed = true;
        }
    }
    if (changed && !saveNoLock(true))
    {
        m_accounts = accounts;
        m_accountIndex = index;
        m_sessions = sessions;
        m_adRewards = rewards;
        m_customRecipes = recipes;
        m_comments = comments;
        m_messages = messages;
        m_videos = videos;
        m_personalizations = personalizations;
        m_sharedPersonalizations = shares;
        m_idempotencyKeys = idempotency;
        m_creationDrafts = drafts;
        m_reviewBases = bases;
        m_recipeReviews = reviews;
        m_creationAliases = aliases;
        m_submissionUsage = usage;
        saveNoLock(true);
        message = "Cannot persist system identity migration";
        return false;
    }
    message = "ok";
    LOGINFO("AccountStore identity initialized systemUserId=qmzdc loginName=%s migrated=%d", name.c_str(), changed ? 1 : 0);
    return true;
}