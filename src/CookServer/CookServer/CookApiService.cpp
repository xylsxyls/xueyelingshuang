#include "CookApiService.h"
#include "CookCatalog.h"
#include "Config.h"
#include "CookSchedule.h"
#include "CookServerHelper.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <map>
#include <set>

std::string CookApiService::jsonFieldString(const std::string& body, const char* key) const
{
	RapidJsonDocument document;
	if (!document.parse(body) || !document.isObject())
	{
		return "";
	}
	return document.getStringOrDefault(key, "");
}

int32_t CookApiService::jsonFieldInt(const std::string& body, const char* key, int32_t defaultValue) const
{
	RapidJsonDocument document;
	if (!document.parse(body) || !document.isObject())
	{
		return defaultValue;
	}
	return document.getIntOrDefault(key, defaultValue);
}

std::vector<std::string> CookApiService::parseRecipeIds(const std::string& body) const
{
	RapidJsonDocument document;
	if (!document.parse(body) || !document.isObject())
	{
		return std::vector<std::string>();
	}
	return document.getStringArrayOrEmpty("recipeIds");
}

std::string CookApiService::requestUserId(const HttpRequest& request) const
{
	std::string userId;
	std::map<std::string, std::string>::const_iterator it = request.m_query.find("userId");
	if (it != request.m_query.end())
	{
		userId = it->second;
	}
	if (userId.empty())
	{
		userId = jsonFieldString(request.m_body, "userId");
	}
	return userId.empty() ? "demo_user" : userId;
}

std::string CookApiService::buildHealthJson() const
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);
	CookServerHelper::addString(document, "service", "CookServer");
	CookServerHelper::addString(document, "version", g_config.m_apiVersion);
	CookServerHelper::addString(document, "publicIp", g_config.m_serverPublicIp);
	CookServerHelper::addString(document, "listenHost", g_config.m_httpListenHost);
	CookServerHelper::addInt(document, "port", static_cast<int32_t>(g_config.m_httpPort));
	return document.toString();
}

std::string CookApiService::buildRecipesJson(const std::string& userId)
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);

	UserAccount account = m_accountStore.getAccount(userId);
	const std::vector<Recipe>& recipes = CookCatalog::recipeCatalog();
	std::set<std::string> categorySet;
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		categorySet.insert(recipes[i].m_category);
	}

	RapidJsonValue categories;
	categories.setArray();
	categories.reserve(categorySet.size() + 3);
	categories.pushString("最喜欢");
	categories.pushString("购买区");
	categories.pushString("创作区");
	for (std::set<std::string>::const_iterator it = categorySet.begin(); it != categorySet.end(); ++it)
	{
		categories.pushString(*it);
	}
	document.addValue("categories", categories);

	RapidJsonValue list;
	list.setArray();
	list.reserve(recipes.size());
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		list.pushValue(CookServerHelper::recipeToJson(recipes[i], &account));
	}
	document.addValue("recipes", list);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildFeedJson(const std::string& userId)
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);

	UserAccount account = m_accountStore.getAccount(userId);
	RapidJsonValue tabs;
	tabs.setArray();
	tabs.reserve(4);
	tabs.pushString("推荐");
	tabs.pushString("精选");
	tabs.pushString("关注");
	tabs.pushString("好友");
	document.addValue("tabs", tabs);

	RapidJsonValue feed;
	feed.setArray();
	const std::vector<Recipe>& recipes = CookCatalog::recipeCatalog();
	feed.reserve(recipes.size());
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "id", "feed_" + recipes[i].m_id);
		CookServerHelper::addString(item, "title", recipes[i].m_title);
		CookServerHelper::addString(item, "description", recipes[i].m_subtitle);
		CookServerHelper::addString(item, "author", recipes[i].m_author);
		CookServerHelper::addString(item, "coverColor", recipes[i].m_coverColor);
		CookServerHelper::addInt(item, "priceCoins", recipes[i].m_priceCoins);
		CookServerHelper::addBool(item, "owned", AccountStore::isRecipeOwned(recipes[i], account));
		CookServerHelper::addBool(item, "favorite", account.m_favoriteRecipeIds.find(recipes[i].m_id) != account.m_favoriteRecipeIds.end());
		CookServerHelper::addInt(item, "likes", 2800 + static_cast<int32_t>(i) * 371);
		CookServerHelper::addInt(item, "comments", 120 + static_cast<int32_t>(i) * 29);

		RapidJsonValue recipeIds;
		recipeIds.setArray();
		recipeIds.reserve(recipes[i].m_id == "sweet_sour_ribs" ? 2 : 1);
		recipeIds.pushString(recipes[i].m_id);
		if (recipes[i].m_id == "sweet_sour_ribs")
		{
			recipeIds.pushString("rice");
		}
		item.addValue("recipeIds", recipeIds);
		feed.pushValue(item);
	}
	document.addValue("feed", feed);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildAccountJson(const std::string& userId)
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);
	document.addValue("account", CookServerHelper::accountToJson(m_accountStore.getAccount(userId)));
	return document.toString();
}

std::string CookApiService::buildRechargeJson(const std::string& userId, int32_t coins)
{
	bool ok = false;
	std::string message;
	UserAccount account = m_accountStore.recharge(userId, coins, &ok, &message);
	if (ok)
	{
		LOGINFO("CookApiService recharge success userId=%s coins=%d balance=%d",
		        userId.c_str(),
		        coins,
		        account.m_coins);
	}
	else
	{
		LOGWARNING("CookApiService recharge failed userId=%s coins=%d message=%s",
		           userId.c_str(),
		           coins,
		           message.c_str());
	}

	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildPurchaseJson(const std::string& userId, const std::string& recipeId)
{
	bool ok = false;
	std::string message;
	UserAccount account = m_accountStore.purchaseRecipe(userId, recipeId, &ok, &message);
	const Recipe* recipe = CookCatalog::findRecipe(recipeId);
	if (ok)
	{
		LOGINFO("CookApiService purchase success userId=%s recipeId=%s balance=%d message=%s",
		        userId.c_str(),
		        recipeId.c_str(),
		        account.m_coins,
		        message.c_str());
	}
	else
	{
		LOGWARNING("CookApiService purchase failed userId=%s recipeId=%s balance=%d message=%s",
		           userId.c_str(),
		           recipeId.c_str(),
		           account.m_coins,
		           message.c_str());
	}

	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	if (recipe != nullptr)
	{
		document.addValue("recipe", CookServerHelper::recipeToJson(*recipe, &account));
	}
	return document.toString();
}

std::string CookApiService::buildFavoriteToggleJson(const std::string& userId, const std::string& recipeId)
{
	bool ok = false;
	bool favorite = false;
	std::string message;
	UserAccount account = m_accountStore.toggleFavorite(userId, recipeId, &ok, &favorite, &message);
	const Recipe* recipe = CookCatalog::findRecipe(recipeId);
	if (ok)
	{
		LOGINFO("CookApiService favorite toggle success userId=%s recipeId=%s favorite=%d",
		        userId.c_str(),
		        recipeId.c_str(),
		        favorite ? 1 : 0);
	}
	else
	{
		LOGWARNING("CookApiService favorite toggle failed userId=%s recipeId=%s message=%s",
		           userId.c_str(),
		           recipeId.c_str(),
		           message.c_str());
	}

	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addBool(document, "favorite", favorite);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	if (recipe != nullptr)
	{
		document.addValue("recipe", CookServerHelper::recipeToJson(*recipe, &account));
	}
	return document.toString();
}

std::string CookApiService::buildPlanJson(const std::vector<std::string>& ids, const std::string& userId, bool includePlanId)
{
	LOGINFO("CookApiService plan request userId=%s recipeCount=%d includePlanId=%d",
	        userId.c_str(),
	        static_cast<int32_t>(ids.size()),
	        includePlanId ? 1 : 0);
	RapidJsonDocument document;
	document.setObject();
	if (ids.empty())
	{
		LOGWARNING("CookApiService plan failed, recipeIds is empty userId=%s includePlanId=%d",
		           userId.c_str(),
		           includePlanId ? 1 : 0);
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "recipeIds is required");
		return document.toString();
	}

	UserAccount account = m_accountStore.getAccount(userId);
	for (size_t i = 0; i < ids.size(); ++i)
	{
		const Recipe* recipe = CookCatalog::findRecipe(ids[i]);
		if (recipe == nullptr)
		{
			LOGWARNING("CookApiService plan denied, recipe not found userId=%s recipeId=%s",
			           userId.c_str(),
			           ids[i].c_str());
			document.addBool("ok", false);
			CookServerHelper::addString(document, "message", "菜谱不存在：" + ids[i]);
			return document.toString();
		}
		if (!AccountStore::isRecipeOwned(*recipe, account))
		{
			LOGWARNING("CookApiService plan denied, recipe not owned userId=%s recipeId=%s recipeTitle=%s",
			           userId.c_str(),
			           ids[i].c_str(),
			           recipe->m_title.c_str());
			document.addBool("ok", false);
			CookServerHelper::addString(document, "message", "请先购买菜谱：" + recipe->m_title);
			return document.toString();
		}
	}

	PlanResult plan = CookSchedule::buildPlan(ids);
	LOGINFO("CookApiService plan success userId=%s recipeCount=%d timelineCount=%d totalSeconds=%d activeSeconds=%d freeSeconds=%d edgeSeconds=%d includePlanId=%d",
	        userId.c_str(),
	        static_cast<int32_t>(plan.m_recipes.size()),
	        static_cast<int32_t>(plan.m_timeline.size()),
	        plan.m_totalSeconds,
	        plan.m_activeSeconds,
	        plan.m_freeSeconds,
	        plan.m_edgeSeconds,
	        includePlanId ? 1 : 0);
	return CookServerHelper::planToJson(plan, account, includePlanId).toString();
}

HttpResponse CookApiService::handle(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	const std::string userId = requestUserId(request);
	LOGINFO("CookApiService request begin method=%s uri=%s userId=%s remoteIp=%s remotePort=%d bodyBytes=%d",
	        request.m_method.c_str(),
	        request.m_uri.c_str(),
	        userId.c_str(),
	        request.m_remoteIp.c_str(),
	        request.m_remotePort,
	        static_cast<int32_t>(request.m_body.size()));

	HttpResponse response;
	if (request.m_method == "OPTIONS")
	{
		response = CookServerHelper::jsonResponse("{\"ok\":true}");
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/health")
	{
		response = CookServerHelper::jsonResponse(buildHealthJson());
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/recipes")
	{
		response = CookServerHelper::jsonResponse(buildRecipesJson(userId));
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/feed")
	{
		response = CookServerHelper::jsonResponse(buildFeedJson(userId));
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/account")
	{
		response = CookServerHelper::jsonResponse(buildAccountJson(userId));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/account/recharge")
	{
		response = CookServerHelper::jsonResponse(buildRechargeJson(userId, jsonFieldInt(request.m_body, "coins", 0)));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/recipes/purchase")
	{
		response = CookServerHelper::jsonResponse(buildPurchaseJson(userId, jsonFieldString(request.m_body, "recipeId")));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/favorites/toggle")
	{
		response = CookServerHelper::jsonResponse(buildFavoriteToggleJson(userId, jsonFieldString(request.m_body, "recipeId")));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/plan/preview")
	{
		response = CookServerHelper::jsonResponse(buildPlanJson(parseRecipeIds(request.m_body), userId, false));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/plan/start")
	{
		response = CookServerHelper::jsonResponse(buildPlanJson(parseRecipeIds(request.m_body), userId, true));
	}
	else
	{
		LOGWARNING("CookApiService request not found method=%s uri=%s userId=%s",
		           request.m_method.c_str(),
		           request.m_uri.c_str(),
		           userId.c_str());
		response = CookServerHelper::jsonResponse("{\"ok\":false,\"message\":\"not found\"}", kHttpStatusNotFound);
	}

	LOGINFO("CookApiService request finish method=%s uri=%s userId=%s status=%d costMs=%d responseBytes=%d",
	        request.m_method.c_str(),
	        request.m_uri.c_str(),
	        userId.c_str(),
	        response.m_statusCode,
	        CSystem::GetHighTickCountMilliRunTime(beginTime),
	        static_cast<int32_t>(response.m_body.size()));
	return response;
}