#include "AccountStore.h"
#include "CookServerHelper.h"
#include "RapidJson/RapidJsonAPI.h"

bool AccountStore::migrateReviewIdentityNoLock(bool &changed, std::string &message)
{
    const std::string oldId = "system", newId = "qmzdc", prefix = oldId + "|";
    const std::string nickname = CookServerHelper::textToJsonUtf8("全民做大厨");
    for (std::map<std::string, std::string>::const_iterator it = m_creationAliases.begin();
        it != m_creationAliases.end(); ++it)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0 &&
            m_creationAliases.count(newId + "|" + it->first.substr(prefix.size())) != 0)
        {
            message = "System creation aliases conflict; migration refused";
            return false;
        }
    }
    for (std::map<std::string, std::map<std::string, int32_t>>::const_iterator it = m_submissionUsage.begin();
        it != m_submissionUsage.end(); ++it)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0 &&
            m_submissionUsage.count(newId + "|" + it->first.substr(prefix.size())) != 0)
        {
            message = "System submission counters conflict; migration refused";
            return false;
        }
    }
    // 先验证并生成新JSON，全部成功后才替换；不改变历史价格、步骤、媒体引用。
    std::map<std::string, std::string> bases, deltas;
    for (std::map<std::string, std::string>::const_iterator it = m_reviewBases.begin(); it != m_reviewBases.end(); ++it)
    {
        RapidJsonDocument json;
        if (!json.parse(it->second))
        {
            return false;
        }
        if (json.getStringOrDefault("authorUserId", "") == oldId)
        {
            json.addString("authorUserId", newId);
            json.addString("author", nickname);
            bases[it->first] = json.toString();
        }
    }
    for (std::map<std::string, CookReviewRevision>::const_iterator it = m_recipeReviews.begin();
        it != m_recipeReviews.end(); ++it)
    {
        RapidJsonDocument json;
        if (!json.parse(it->second.m_deltaJson))
        {
            return false;
        }
        std::vector<RapidJsonValue> fields = json.getArrayValueOrEmpty("fields");
        bool modified = false;
        RapidJsonValue values;
        values.setArray();
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const std::string field = fields[i].getStringOrDefault("field", "");
            if (field == "authorUserId" && fields[i].getStringOrDefault("value", "") == oldId)
            {
                fields[i].addString("value", newId);
                modified = true;
            }
            else if (field == "author" && it->second.m_ownerUserId == oldId)
            {
                fields[i].addString("value", nickname);
                modified = true;
            }
            values.pushValue(fields[i]);
        }
        if (modified)
        {
            json.addValue("fields", values);
            deltas[it->first] = json.toString();
        }
    }
    for (std::map<std::string, std::string>::const_iterator it = bases.begin(); it != bases.end(); ++it)
    {
        m_reviewBases[it->first] = it->second;
    }
    for (std::map<std::string, std::string>::const_iterator it = deltas.begin(); it != deltas.end(); ++it)
    {
        m_recipeReviews[it->first].m_deltaJson = it->second;
    }
    changed = changed || !bases.empty() || !deltas.empty();
    for (std::map<std::string, Recipe>::iterator it = m_creationDrafts.begin(); it != m_creationDrafts.end(); ++it)
    {
        if (it->second.m_authorUserId == oldId)
        {
            it->second.m_authorUserId = newId;
            it->second.m_author = nickname;
            changed = true;
        }
    }
    for (std::map<std::string, CookReviewRevision>::iterator it = m_recipeReviews.begin(); it != m_recipeReviews.end();
        ++it)
    {
        if (it->second.m_ownerUserId == oldId)
        {
            it->second.m_ownerUserId = newId;
            changed = true;
        }
        if (it->second.m_reviewerUserId == oldId)
        {
            it->second.m_reviewerUserId = newId;
            changed = true;
        }
    }
    for (std::map<std::string, std::string>::iterator it = m_creationAliases.begin(); it != m_creationAliases.end();)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0)
        {
            const std::string key = newId + "|" + it->first.substr(prefix.size()), value = it->second;
            m_creationAliases.erase(it++);
            m_creationAliases[key] = value;
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    for (std::map<std::string, std::map<std::string, int32_t>>::iterator it = m_submissionUsage.begin();
        it != m_submissionUsage.end();)
    {
        if (it->first.compare(0, prefix.size(), prefix) == 0)
        {
            const std::string key = newId + "|" + it->first.substr(prefix.size());
            const std::map<std::string, int32_t> value = it->second;
            m_submissionUsage.erase(it++);
            m_submissionUsage[key] = value;
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    return true;
}