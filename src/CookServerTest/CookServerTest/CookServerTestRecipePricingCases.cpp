#include "CookServerTestRecipeEditingCases.h"
#include "CookServerTestConfig.h"

void CookServerTestRecipeEditingCases::runPricingCases(CookServerTestHttpClient& client,
    const CookServerTestFeedCases::RegisterFunction& registerCase)
{
    registerCase("作者定价免费金币元宝及金额类型边界", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument user, result;
        RapidJsonValue recipe;
        if (!fixture(client, "pricing", user, recipe, message))
        {
            return false;
        }
        std::string token = user.getStringOrDefault("token", "");
        const char* types[] = { "coin", "yuanbao", "free", "coin" };
        const int32_t amounts[] = { 1, 19, 0, 2147483647 };
        for (size_t i = 0; i < 4; ++i)
        {
            if (i == 3) { if (!fixture(client, "pricing_max", user, recipe, message)) return false; token = user.getStringOrDefault("token", ""); }
            RapidJsonDocument request = body(recipe, "Chop", 31);
            request.addString("priceType", types[i]);
            request.addInt("priceAmount", amounts[i]);
            if (!call(client, token, "/api/recipes/creation/save", request.toString(), result, message))
            {
                return false;
            }
            recipe = result.getValue("recipe");
            if (recipe.getStringOrDefault("priceType", "") != types[i] || recipe.getIntOrDefault("priceAmount", -1) != amounts[i] ||
                recipe.getIntOrDefault("priceCoins", -1) != (i == 0 || i == 3 ? amounts[i] : 0))
            {
                message = "Price type, amount or legacy coin amount differs";
                return false;
            }
        }
        for (int32_t variant = 0; variant < 14; ++variant)
        {
            RapidJsonDocument request = body(recipe, "Chop", 31);
            if (variant == 0 || variant == 1 || variant == 2 || variant == 3 || variant == 4 || variant == 13)
            {
                request.addString("priceType", variant == 1 ? "yuanbao" : (variant == 3 ? "free" : (variant == 4 ? "cash" : (variant == 13 ? "" : "coin"))));
                request.addInt("priceAmount", variant == 2 ? -1 : (variant == 3 || variant == 4 || variant == 13 ? 1 : 0));
            }
            else if (variant == 5)
            {
                request.addInt("priceType", 123);
            }
            else if (variant == 6)
            {
                request.addString("priceAmount", "7");
            }
            else if (variant == 7 || variant == 8 || variant == 11 || variant == 12)
            {
                RapidJsonDocument literal;
                literal.parse(variant == 7 ? "{\"value\":0.5}" : (variant == 8 ? "{\"value\":2147483648}" : (variant == 11 ? "{\"value\":null}" : "{\"value\":false}")));
                request.addValue("priceAmount", literal.getValue("value"));
                if (variant == 11)
                {
                    request.addValue("priceType", literal.getValue("value"));
                }
            }
            else
            {
                RapidJsonDocument partial;
                partial.setObject();
                partial.addString("recipeId", recipe.getStringOrDefault("id", ""));
                partial.addString("currentRevisionId", recipe.getStringOrDefault("currentRevisionId", ""));
                partial.addValue("steps", request.getValue("steps"));
                if (variant == 9)
                {
                    partial.addString("priceType", "coin");
                }
                else
                {
                    partial.addInt("priceAmount", 3);
                }
                request = partial;
            }
            if (!call(client, token, "/api/recipes/creation/save", request.toString(), result, message, false))
            {
                return false;
            }
        }
        if (!call(client, token, "/api/recipes/creation/detail?recipeId=" + recipe.getStringOrDefault("id", ""), "", result, message) ||
            result.getValue("recipe").getStringOrDefault("currentRevisionId", "") != recipe.getStringOrDefault("currentRevisionId", "") ||
            result.getValue("recipe").getIntOrDefault("priceAmount", -1) != 2147483647)
        {
            message = "Rejected price changed the stored recipe";
            return false;
        }
        RapidJsonDocument legacy;
        legacy.setObject();
        legacy.addString("recipeId", recipe.getStringOrDefault("id", ""));
        legacy.addString("currentRevisionId", recipe.getStringOrDefault("currentRevisionId", ""));
        legacy.addValue("steps", body(recipe, "Chop", 31).getValue("steps"));
        if (!call(client, token, "/api/recipes/creation/save", legacy.toString(), result, message) ||
            result.getValue("recipe").getIntOrDefault("priceAmount", -1) != 2147483647)
        {
            message = "Legacy client did not preserve price";
            return false;
        }
        status = 200;
        return true;
    });
    registerCase("系统菜谱改收费后不自动授予新用户且购买权益保留", [&](std::string& message, int32_t& status) -> bool
    {
        RapidJsonDocument login, source, saved, buyer, purchase, catalog;
        if (!call(client, "", "/api/auth/login", "{\"account\":\"qmzdc\",\"password\":\"yangnan\"}", login, message))
        {
            return false;
        }
        const std::string token = login.getStringOrDefault("token", "");
        if (!call(client, token, "/api/recipes/creation/detail?recipeId=cook_000001", "", source, message))
        {
            return false;
        }
        const auto approveSaved = [&]() -> bool { RapidJsonDocument body, result; body.addString("reviewId", saved.getValue("recipe").getStringOrDefault("reviewId", "")); body.addString("decision", "approved"); return call(client, token, "/api/recipes/reviews/decide", body.toString(), result, message); };
        const RapidJsonValue original = source.getValue("recipe");
        RapidJsonDocument request = body(original, "Chop", 31);
        request.addString("priceType", "coin");
        request.addInt("priceAmount", 7);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message) || !approveSaved())
        {
            return false;
        }
        const std::string account = g_cookServerTestConfig.m_userIdPrefix + "_pricing_buyer";
        if (!call(client, "", "/api/auth/register", "{\"account\":\"" + account + "\",\"password\":\"PriceTest123!\",\"nickname\":\"Buyer\"}", buyer, message))
        {
            return false;
        }
        const std::string buyerToken = buyer.getStringOrDefault("token", "");
        if (!call(client, buyerToken, "/api/recipes?scope=catalog", "", catalog, message))
        {
            return false;
        }
        bool found = false;
        const std::vector<RapidJsonValue> rows = catalog.getArrayValueOrEmpty("recipes");
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (rows[i].getStringOrDefault("id", "") != "cook_000001")
            {
                continue;
            }
            found = true;
            if (rows[i].getBoolOrDefault("owned", true) || rows[i].getBoolOrDefault("inMenu", true) || rows[i].getBoolOrDefault("accessible", true))
            {
                message = "Repriced system recipe auto-granted access or menu membership";
                return false;
            }
        }
        if (!found || saved.getValue("recipe").getArrayValueOrEmpty("tasks").empty())
        {
            message = "Catalog recipe or author's original steps missing";
            return false;
        }
        if (!call(client, buyerToken, "/api/recipes/purchase", "{\"recipeId\":\"cook_000001\",\"idempotencyKey\":\"pricing-purchase\"}", purchase, message))
        {
            return false;
        }
        request = body(saved.getValue("recipe"), "Chop", 31);
        request.addString("priceType", "yuanbao");
        request.addInt("priceAmount", 19);
        if (!call(client, token, "/api/recipes/creation/save", request.toString(), saved, message) ||
            !approveSaved() || !call(client, buyerToken, "/api/recipes/detail?recipeId=cook_000001", "", source, message))
        {
            return false;
        }
        if (!source.getValue("recipe").getBoolOrDefault("owned", false) ||
            !source.getValue("recipe").getBoolOrDefault("purchased", false) || source.getValue("recipe").getArrayValueOrEmpty("tasks").empty())
        {
            message = "Changing price removed a buyer's access";
            return false;
        }
        RapidJsonValue restore = original;
        restore.addString("currentRevisionId", saved.getValue("recipe").getStringOrDefault("currentRevisionId", ""));
        const RapidJsonValue first = restore.getArrayValueOrEmpty("tasks")[0];
        if (!call(client, token, "/api/recipes/creation/save", body(restore, first.getStringOrDefault("title", ""),
            first.getIntOrDefault("durationSeconds", -1)).toString(), saved, message))
        {
            return false;
        }
        status = 200;
        return true;
    });
}