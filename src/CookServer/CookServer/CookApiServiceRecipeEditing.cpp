#include "CookApiService.h"
#include "CookServerHelper.h"
#include "CookHelper.h"
#include "RapidJson/RapidJsonAPI.h"

std::string CookApiService::buildCreationJson(const HttpRequest& request, const std::string& userId)
{
    Recipe recipe;
    std::string message;
    bool ok = false;
    if (request.m_method == "GET")
    {
        ok = m_accountStore.getCreationRecipe(userId, CookHelper::queryValue(request, "recipeId"), recipe, message);
    }
    else
    {
        RapidJsonDocument body;
        std::vector<CookTask> steps;
        std::string priceType;
        int32_t priceAmount = -1;
        int32_t editorVersion = 1;
        bool valid = parseBody(request.m_body, body) && body.getValue("steps").isArray();
        if (valid)
        {
            const std::vector<RapidJsonValue> values = body.getArrayValueOrEmpty("steps");
            const std::string missing(1, '\x01');
            valid = values.size() <= 256;
            editorVersion = body.getIntOrDefault("editorVersion", 0);
            if (editorVersion == 0)
            {
                RapidJson members;
                valid = valid && members.parse(request.m_body) && !members.hasMember("editorVersion");
                editorVersion = 1;
            }
            valid = valid && (editorVersion == 1 || editorVersion == 2);
            bool hasPrice = body.getStringOrDefault("priceType", missing) != missing || body.getIntOrDefault("priceAmount", -1) != -1;
            if (!hasPrice)
            {
                // 兼容未提交定价的旧客户端，同时区分字段缺失与显式非法值。
                RapidJson members;
                valid = valid && members.parse(request.m_body);
                hasPrice = members.hasMember("priceType") || members.hasMember("priceAmount");
            }
            if (hasPrice)
            {
                priceType = body.getStringOrDefault("priceType", missing);
                priceAmount = body.getIntOrDefault("priceAmount", -1);
                valid = valid && priceType != missing && !priceType.empty() && priceAmount >= 0;
            }
            for (size_t i = 0; i < values.size(); ++i)
            {
                if (!valid || !values[i].isObject() || values[i].getStringOrDefault("id", missing) == missing ||
                    values[i].getStringOrDefault("title", missing) == missing || values[i].getStringOrDefault("detail", missing) == missing ||
                    values[i].getIntOrDefault("durationSeconds", -1) < 0)
                {
                    valid = false;
                    break;
                }
                CookTask step;
                step.m_id = values[i].getStringOrDefault("id", "");
                step.m_title = values[i].getStringOrDefault("title", "");
                step.m_detail = values[i].getStringOrDefault("detail", "");
                step.m_durationSeconds = values[i].getIntOrDefault("durationSeconds", -1);
                if (editorVersion == 2)
                {
                    const bool withTrue = values[i].getBoolOrDefault("active", true);
                    const bool withFalse = values[i].getBoolOrDefault("active", false);
                    if (withTrue != withFalse)
                    {
                        valid = false;
                        break;
                    }
                    step.m_active = withFalse;
                    step.m_safetyLevel = values[i].getStringOrDefault("safetyLevel", "normal");
                    step.m_backgroundWaitMode = values[i].getStringOrDefault("backgroundWaitMode", "watch");
                    step.m_canLeaveKitchen = values[i].getBoolOrDefault("canLeaveKitchen", false);
                }
                steps.push_back(step);
            }
        }
        if (valid)
        {
            std::vector<std::string> mediaIds;
            std::string coverImageId;
            RapidJson members;
            const bool parsedMembers = members.parse(request.m_body);
            const bool mediaSpecified = parsedMembers && members.hasMember("mediaIds");
            const bool coverSpecified = parsedMembers && members.hasMember("coverImageId");
            if (mediaSpecified)
            {
                mediaIds = body.getStringArrayOrEmpty("mediaIds");
                valid = body.getValue("mediaIds").isArray() && mediaIds.size() == body.getValue("mediaIds").size() && mediaIds.size() <= 9;
            }
            if (coverSpecified)
            {
                const std::string missing(1, '\x01');
                coverImageId = body.getStringOrDefault("coverImageId", missing);
                valid = valid && coverImageId != missing;
            }
            if (valid)
            {
                ok = m_accountStore.saveCreationSteps(userId, body.getStringOrDefault("recipeId", ""),
                    body.getStringOrDefault("currentRevisionId", ""), steps, priceType, priceAmount, editorVersion, recipe, message,
                    mediaIds, mediaSpecified, coverImageId, coverSpecified);
            }
            else
            {
                message = "Invalid creation media references";
            }
        }
        else
        {
            message = "Invalid recipe step payload";
        }
    }
    RapidJsonDocument result;
    result.setObject();
    result.addBool("ok", ok);
    CookServerHelper::addString(result, "message", message);
    if (ok)
    {
        const UserAccount account = m_accountStore.getAccount(userId);
        result.addValue("recipe", CookServerHelper::recipeToJson(recipe, &account));
    }
    return result.toString();
}