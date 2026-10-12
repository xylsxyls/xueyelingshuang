#include "CookServerTestRecipeEditingCases.h"

#include <map>

RapidJsonDocument CookServerTestRecipeEditingCases::structureBody(const RapidJsonValue& recipe)
{
    const std::vector<RapidJsonValue> original = recipe.getArrayValueOrEmpty("tasks");
    RapidJsonDocument request = body(recipe, original.empty() ? "Step" : original[0].getStringOrDefault("title", ""),
        original.empty() ? 0 : original[0].getIntOrDefault("durationSeconds", 0));
    request.addInt("editorVersion", 2);
    std::vector<RapidJsonValue> values = request.getArrayValueOrEmpty("steps");
    RapidJsonValue steps;
    steps.setArray();
    for (size_t i = 0; i < values.size(); ++i)
    {
        values[i].addBool("active", original[i].getBoolOrDefault("active", false));
        steps.pushValue(values[i]);
    }
    request.addValue("steps", steps);
    return request;
}

void CookServerTestRecipeEditingCases::runStructureCases(CookServerTestHttpClient& client,
    const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("作者新增删除排序及步骤性质实际保存和排程", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument user, saved, plan;
        RapidJsonValue recipe;
        if (!fixture(client, "structure", user, recipe, message))
        {
            return false;
        }
        const std::string token = user.getStringOrDefault("token", ""), id = recipe.getStringOrDefault("id", "");
        RapidJsonDocument request = structureBody(recipe);
        std::vector<RapidJsonValue> original = request.getArrayValueOrEmpty("steps");
        RapidJsonValue added;
        added.setObject();
        added.addString("id", id + ".added_mix");
        added.addString("title", "Mix");
        added.addString("detail", "Mix evenly");
        added.addInt("durationSeconds", 15);
        added.addBool("active", true);
        RapidJsonValue steps;
        steps.setArray();
        steps.pushValue(original[1]);
        steps.pushValue(added);
        steps.pushValue(original[0]);
        request.addValue("steps", steps);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message))
        {
            return false;
        }
        const std::vector<RapidJsonValue> first = saved.getValue("recipe").getArrayValueOrEmpty("tasks");
        if (first.size() != 3 || first[0].getStringOrDefault("id", "") != id + ".cook" ||
            first[1].getStringOrDefault("id", "") != id + ".added_mix" || first[2].getStringOrDefault("id", "") != id + ".cut")
        {
            message = "First reordered list differs";
            return false;
        }
        request = structureBody(saved.getValue("recipe"));
        std::vector<RapidJsonValue> current = request.getArrayValueOrEmpty("steps");
        current[0].addBool("active", true);
        RapidJsonValue hot = added;
        hot.addString("id", id + ".added_hot");
        hot.addString("title", "Boil");
        hot.addBool("active", false);
        hot.addString("safetyLevel", "danger");
        hot.addString("backgroundWaitMode", "free");
        hot.addBool("canLeaveKitchen", true);
        steps.setArray();
        steps.pushValue(current[1]);
        steps.pushValue(current[0]);
        steps.pushValue(hot);
        request.addValue("steps", steps);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message) ||
            !call(client, token, "/api/plan/preview", "{\"recipeIds\":[\"" + id + "\"]}", plan, message))
        {
            return false;
        }
        const std::vector<RapidJsonValue> tasks = saved.getValue("recipe").getArrayValueOrEmpty("tasks");
        if (tasks.size() != 3 || tasks[0].getStringOrDefault("id", "") != id + ".added_mix" ||
            tasks[1].getStringOrDefault("id", "") != id + ".cook" || !tasks[1].getBoolOrDefault("active", false) ||
            tasks[1].getStringOrDefault("resource", "") != "stove" || tasks[2].getBoolOrDefault("canLeaveKitchen", true) ||
            tasks[2].getStringOrDefault("backgroundWaitMode", "") != "watch" || tasks[2].getStringOrDefault("safetyLevel", "") != "danger")
        {
            message = "Deleted step, type, resource or danger guard differs";
            return false;
        }
        const std::vector<RapidJsonValue> timeline = plan.getArrayValueOrEmpty("timeline");
        std::map<std::string, RapidJsonValue> byId;
        for (size_t i = 0; i < timeline.size(); ++i)
        {
            byId[timeline[i].getStringOrDefault("id", "")] = timeline[i];
        }
        if (timeline.size() != 3 || byId[id + ".cook"].getIntOrDefault("startSeconds", -1) <
            byId[id + ".added_mix"].getIntOrDefault("endSeconds", 999999) || byId[id + ".added_hot"].getIntOrDefault("startSeconds", -1) <
            byId[id + ".cook"].getIntOrDefault("endSeconds", 999999))
        {
            message = "Scheduled order does not respect edited foreground sequence";
            return false;
        }
        status = 200;
        return true;
    });
    registerCase("结构编辑拒绝空列表重复跨菜谱ID非法类型版本及越权", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument owner, peer, result;
        RapidJsonValue recipe, peerRecipe;
        if (!fixture(client, "structure_boundary", owner, recipe, message) || !fixture(client, "structure_peer", peer, peerRecipe, message))
        {
            return false;
        }
        const std::string token = owner.getStringOrDefault("token", ""), id = recipe.getStringOrDefault("id", "");
        for (int variant = 0; variant < 9; ++variant)
        {
            RapidJsonDocument request = structureBody(recipe);
            std::vector<RapidJsonValue> values = request.getArrayValueOrEmpty("steps");
            if (variant == 0)
            {
                values.clear();
            }
            else if (variant == 1)
            {
                values[1] = values[0];
            }
            else if (variant == 2)
            {
                values[0].addString("id", "another.added_x");
            }
            else if (variant == 3)
            {
                values[0].addString("active", "true");
            }
            else if (variant == 4)
            {
                request.addInt("editorVersion", 3);
            }
            else if (variant == 5)
            {
                values.resize(257, values[0]);
            }
            else if (variant == 6)
            {
                values[0].addString("id", id + ".added_../bad");
            }
            else if (variant == 7)
            {
                request.addString("editorVersion", "2");
            }
            else
            {
                request.addString("currentRevisionId", "expired");
            }
            RapidJsonValue steps;
            steps.setArray();
            for (size_t i = 0; i < values.size(); ++i)
            {
                steps.pushValue(values[i]);
            }
            request.addValue("steps", steps);
            if (!call(client, token, "/api/recipes/creation/save", request.toString(), result, message, false))
            {
                return false;
            }
        }
        if (!call(client, peer.getStringOrDefault("token", ""), "/api/recipes/creation/save", structureBody(recipe).toString(), result, message, false) ||
            !call(client, token, "/api/recipes/creation/detail?recipeId=" + id, "", result, message) ||
            result.getValue("recipe").getStringOrDefault("currentRevisionId", "") != recipe.getStringOrDefault("currentRevisionId", ""))
        {
            message = "Rejected structure changed source or allowed another author";
            return false;
        }
        status = 200;
        return true;
    });
    registerCase("作者结构变更保留有效个性化设置并拒绝个人新增ID冲突", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument user, saved, personal, share;
        RapidJsonValue recipe;
        if (!fixture(client, "structure_personal", user, recipe, message))
        {
            return false;
        }
        const std::string token = user.getStringOrDefault("token", ""), id = recipe.getStringOrDefault("id", "");
        const std::string config = "{\"recipeId\":\"" + id + "\",\"durationOverrides\":[{\"taskId\":\"cut\",\"durationSeconds\":47}],\"disabledTaskIds\":[\"cook\"],\"addedTasks\":[{\"id\":\"added_personal\",\"title\":\"Check\",\"durationSeconds\":9,\"active\":true}],\"taskOrder\":[\"cut\",\"added_personal\",\"cook\"]}";
        if (!call(client, token, "/api/recipes/personalization/save", config, personal, message) ||
            !call(client, token, "/api/recipes/personalization/share", "{\"recipeId\":\"" + id + "\"}", share, message))
        {
            return false;
        }
        RapidJsonDocument request = structureBody(recipe);
        const std::vector<RapidJsonValue> original = request.getArrayValueOrEmpty("steps");
        RapidJsonValue added = original[0];
        added.addString("id", id + ".added_creator");
        RapidJsonValue steps;
        steps.setArray();
        steps.pushValue(original[1]);
        steps.pushValue(added);
        steps.pushValue(original[0]);
        request.addValue("steps", steps);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message) ||
            !call(client, token, "/api/recipes/personalization/detail?recipeId=" + id, "", personal, message))
        {
            return false;
        }
        const RapidJsonValue rebased = personal.getValue("personalization");
        if (rebased.getArrayValueOrEmpty("addedTasks").size() != 1 || rebased.getArrayValueOrEmpty("durationOverrides").size() != 1 ||
            rebased.getStringArrayOrEmpty("taskOrder").size() != 4 || rebased.getStringArrayOrEmpty("disabledTaskIds").size() != 1)
        {
            message = "Existing personal settings lost during reorder";
            return false;
        }
        request = structureBody(saved.getValue("recipe"));
        std::vector<RapidJsonValue> current = request.getArrayValueOrEmpty("steps");
        current[1].addString("id", id + ".added_personal");
        steps.setArray();
        for (size_t i = 0; i < current.size(); ++i)
        {
            steps.pushValue(current[i]);
        }
        request.addValue("steps", steps);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), personal, message, false) ||
            !call(client, token, "/api/recipes/creation/detail?recipeId=" + id, "", personal, message) ||
            personal.getValue("recipe").getStringOrDefault("currentRevisionId", "") != saved.getValue("recipe").getStringOrDefault("currentRevisionId", ""))
        {
            message = "Personal ID conflict did not preserve original revision";
            return false;
        }
        status = 200;
        return true;
    });
}