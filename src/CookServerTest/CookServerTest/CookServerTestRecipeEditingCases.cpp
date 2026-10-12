#include "CookServerTestRecipeEditingCases.h"
#include "CookServerTestConfig.h"
#include <map>

bool CookServerTestRecipeEditingCases::call(CookServerTestHttpClient& client, const std::string& token,
    const std::string& path, const std::string& body, RapidJsonDocument& json, std::string& message, bool expectedOk)
{
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer " + token;
    CookServerTestHttpResponse response;
    client.request(body.empty() ? "GET" : "POST", path, body, headers, response);
    if (!response.m_networkOk || response.m_statusCode != 200 || !json.parse(response.m_body) ||
        json.getBoolOrDefault("ok", !expectedOk) != expectedOk)
    {
        message = "Recipe editing HTTP or business result differs: " + response.m_body;
        return false;
    }
    return true;
}

bool CookServerTestRecipeEditingCases::fixture(CookServerTestHttpClient& client, const std::string& suffix,
    RapidJsonDocument& user, RapidJsonValue& recipe, std::string& message)
{
    const std::string id = g_cookServerTestConfig.m_userIdPrefix + "_creation_" + suffix;
    if (!call(client, "", "/api/auth/register", "{\"account\":\"" + id +
        "\",\"password\":\"CreatorTest123!\",\"nickname\":\"Creator\"}", user, message))
    {
        return false;
    }
    RapidJsonDocument result;
    const std::string token = user.getStringOrDefault("token", "");
    const std::string source = "{\"recipe\":{\"schemaVersion\":\"1.0\",\"id\":\"" + id +
        "\",\"title\":\"Editing fixture\",\"category\":\"vegetable\",\"priceType\":\"free\",\"ingredients\":[{\"name\":\"Potato\",\"amount\":\"100\\u514b\"}],\"tasks\":[{\"id\":\"cut\",\"title\":\"Cut\",\"detail\":\"Cut carefully\",\"durationSeconds\":10,\"active\":true},{\"id\":\"cook\",\"title\":\"Cook\",\"detail\":\"Watch stove\",\"durationSeconds\":20,\"active\":false,\"resource\":\"stove\",\"dependsOn\":[\"cut\"]}]}}";
    if (!call(client, token, "/api/recipes/custom/publish", source, result, message))
    {
        return false;
    }
    const std::string serverId = result.getValue("recipe").getStringOrDefault("id", "");
    if (serverId.empty() || !call(client, token, "/api/recipes/creation/detail?recipeId=" + serverId, "", result, message))
    {
        return false;
    }
    recipe = result.getValue("recipe");
    return true;
}

RapidJsonDocument CookServerTestRecipeEditingCases::body(const RapidJsonValue& recipe, const std::string& title, int32_t seconds)
{
    RapidJsonDocument result;
    result.setObject();
    result.addString("recipeId", recipe.getStringOrDefault("id", ""));
    result.addString("currentRevisionId", recipe.getStringOrDefault("currentRevisionId", ""));
    RapidJsonValue steps;
    steps.setArray();
    const std::vector<RapidJsonValue> tasks = recipe.getArrayValueOrEmpty("tasks");
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        RapidJsonValue step;
        step.setObject();
        step.addString("id", tasks[i].getStringOrDefault("id", ""));
        step.addString("title", i == 0 ? title : tasks[i].getStringOrDefault("title", ""));
        step.addString("detail", tasks[i].getStringOrDefault("detail", ""));
        step.addInt("durationSeconds", i == 0 ? seconds : tasks[i].getIntOrDefault("durationSeconds", -1));
        step.addString("resource", "forged-resource");
        steps.pushValue(step);
    }
    result.addValue("steps", steps);
    result.addString("priceType", recipe.getStringOrDefault("priceType", "free"));
    result.addInt("priceAmount", recipe.getIntOrDefault("priceAmount", 0));
    result.addString("authorUserId", "forged-author");
    return result;
}

void CookServerTestRecipeEditingCases::run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("作者编辑原始步骤且不改变价格、依赖和归属", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument user, saved;
        RapidJsonValue original;
        if (!fixture(client, "save", user, original, message))
        {
            return false;
        }
        const std::string token = user.getStringOrDefault("token", "");
        RapidJsonDocument request = body(original, "Chop", 31);
        std::vector<RapidJsonValue> editedSteps = request.getArrayValueOrEmpty("steps");
        editedSteps[0].addString("detail", "Updated explanation");
        RapidJsonValue editedArray;
        editedArray.setArray();
        for (size_t i = 0; i < editedSteps.size(); ++i)
        {
            editedArray.pushValue(editedSteps[i]);
        }
        request.addValue("steps", editedArray);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message))
        {
            return false;
        }
        const RapidJsonValue recipe = saved.getValue("recipe");
        const std::vector<RapidJsonValue> tasks = recipe.getArrayValueOrEmpty("tasks");
        const std::vector<RapidJsonValue> oldTasks = original.getArrayValueOrEmpty("tasks");
        if (tasks.size() != 2 || tasks[0].getStringOrDefault("title", "") != "Chop" ||
            tasks[0].getStringOrDefault("voiceText", "") != "Chop" || tasks[0].getIntOrDefault("durationSeconds", -1) != 31 ||
            tasks[0].getStringOrDefault("detail", "") != "Updated explanation" ||
            tasks[1].getStringOrDefault("resource", "") != oldTasks[1].getStringOrDefault("resource", "") ||
            tasks[1].getValue("dependsOn").toString() != oldTasks[1].getValue("dependsOn").toString() ||
            recipe.getStringOrDefault("authorUserId", "") != original.getStringOrDefault("authorUserId", "") ||
            recipe.getIntOrDefault("priceAmount", -1) != original.getIntOrDefault("priceAmount", -1) ||
            recipe.getStringOrDefault("currentRevisionId", "") == original.getStringOrDefault("currentRevisionId", ""))
        {
            message = "Edited fields or preserved fields differ";
            return false;
        }
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message, false))
        {
            return false;
        }
        status = 200;
        return true;
    });
    registerCase("作者编辑拒绝越权、非法字段和过期版本且旧菜谱不变", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument owner, peer, result;
        RapidJsonValue recipe, peerRecipe;
        if (!fixture(client, "boundary", owner, recipe, message) || !fixture(client, "peer", peer, peerRecipe, message))
        {
            return false;
        }
        const std::string path = "/api/recipes/creation/detail?recipeId=" + recipe.getStringOrDefault("id", "");
        const std::string token = owner.getStringOrDefault("token", "");
        if (!call(client, peer.getStringOrDefault("token", ""), path, "", result, message, false) ||
            !call(client, peer.getStringOrDefault("token", ""), "/api/recipes/creation/save", body(recipe, "Chop", 31).toString(), result, message, false) ||
            !call(client, token, "/api/recipes/creation/save", body(recipe, "123456789", 31).toString(), result, message, false) ||
            !call(client, token, "/api/recipes/creation/save", body(recipe, "Chop", -1).toString(), result, message, false) ||
            !call(client, token, "/api/recipes/creation/save", body(recipe, "Chop", 86401).toString(), result, message, false))
        {
            return false;
        }
        RapidJsonDocument missing = body(recipe, "Chop", 10);
        RapidJsonValue duplicate;
        duplicate.setArray();
        const RapidJsonValue first = missing.getArrayValueOrEmpty("steps")[0];
        duplicate.pushValue(first);
        duplicate.pushValue(first);
        missing.addValue("steps", duplicate);
        if (!call(client, token, "/api/recipes/creation/save", missing.toString(), result, message, false) ||
            !call(client, token, path, "", result, message))
        {
            return false;
        }
        if (result.getValue("recipe").getStringOrDefault("currentRevisionId", "") != recipe.getStringOrDefault("currentRevisionId", ""))
        {
            message = "Rejected edit changed prior recipe";
            return false;
        }
        for (int32_t variant = 0; variant < 5; ++variant)
        {
            RapidJsonDocument invalid = body(recipe, "Chop", 10);
            std::vector<RapidJsonValue> items = invalid.getArrayValueOrEmpty("steps");
            if (variant == 0)
            {
                items[0].addInt("title", 123);
            }
            else if (variant == 1)
            {
                items[0].addString("durationSeconds", "31");
            }
            else if (variant == 2)
            {
                items[0].addString("id", "unknown");
            }
            else if (variant == 3)
            {
                items.resize(1);
            }
            else
            {
                items[0].addString("detail", std::string(8193, 'x'));
            }
            RapidJsonValue list;
            list.setArray();
            for (size_t i = 0; i < items.size(); ++i)
            {
                list.pushValue(items[i]);
            }
            invalid.addValue("steps", list);
            if (!call(client, token, "/api/recipes/creation/save", invalid.toString(), result, message, false))
            {
                return false;
            }
        }
        if (!call(client, token, path, "", result, message) ||
            result.getValue("recipe").getStringOrDefault("currentRevisionId", "") != recipe.getStringOrDefault("currentRevisionId", ""))
        {
            message = "Invalid field payload changed original recipe";
            return false;
        }
        if (!call(client, token, "/api/recipes/creation/save", body(recipe, "12345678", 86400).toString(), result, message))
        {
            return false;
        }
        status = 200;
        return true;
    });
    registerCase("系统作者编辑内置菜谱无重复行并保留系统身份", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument login, source, saved, catalog;
        if (!call(client, "", "/api/auth/login", "{\"account\":\"qmzdc\",\"password\":\"yangnan\"}", login, message))
        {
            return false;
        }
        const std::string token = login.getStringOrDefault("token", "");
        if (!call(client, token, "/api/recipes/creation/detail?recipeId=cook_000001", "", source, message) ||
            !call(client, token, "/api/recipes/creation/save", body(source.getValue("recipe"), "Updated", 30).toString(), saved, message) ||
            !call(client, token, "/api/recipes?scope=catalog", "", catalog, message))
        {
            return false;
        }
        int32_t count = 0;
        const std::vector<RapidJsonValue> recipes = catalog.getArrayValueOrEmpty("recipes");
        for (size_t i = 0; i < recipes.size(); ++i)
        {
            if (recipes[i].getStringOrDefault("id", "") == "cook_000001")
            {
                ++count;
            }
        }
        if (count != 1 || !saved.getValue("recipe").getBoolOrDefault("systemRecipe", false) ||
            saved.getValue("recipe").getStringOrDefault("authorUserId", "") != "qmzdc")
        {
            message = "System edit lost identity or created duplicate catalog entry";
            return false;
        }
        RapidJsonValue restored = source.getValue("recipe");
        restored.addString("currentRevisionId", saved.getValue("recipe").getStringOrDefault("currentRevisionId", ""));
        const RapidJsonValue originalStep = restored.getArrayValueOrEmpty("tasks")[0];
        if (!call(client, token, "/api/recipes/creation/save", body(restored, originalStep.getStringOrDefault("title", ""),
            originalStep.getIntOrDefault("durationSeconds", -1)).toString(), saved, message))
        {
            return false;
        }
        status = 200;
        return true;
    });
    runPricingCases(client, registerCase);
    runStructureCases(client, registerCase);
}