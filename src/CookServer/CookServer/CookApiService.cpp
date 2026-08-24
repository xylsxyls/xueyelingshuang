#include "CookApiService.h"
#include "Config.h"
#include "CookHelper.h"
#include "CookRecipeConfig.h"
#include "CookSchedule.h"
#include "CookServerHelper.h"
#include "CookStorageService.h"
#include "CookVoiceService.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <stdint.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

std::string CookApiService::headerValue(const HttpRequest& request, const std::string& headerName) const
{
	std::string expected = CookHelper::lowerAscii(headerName);
	for (std::map<std::string, std::string>::const_iterator it = request.m_headers.begin(); it != request.m_headers.end(); ++it)
	{
		if (CookHelper::lowerAscii(it->first) == expected)
		{
			return it->second;
		}
	}
	return "";
}

std::string CookApiService::bearerToken(const HttpRequest& request) const
{
	std::string authorization = headerValue(request, "Authorization");
	std::string prefix = "Bearer ";
	if (authorization.size() > prefix.size() && authorization.substr(0, prefix.size()) == prefix)
	{
		return authorization.substr(prefix.size());
	}
	return authorization;
}

bool CookApiService::parseBody(const std::string& body, RapidJsonDocument& document) const
{
	return document.parse(body) && document.isObject();
}

std::string CookApiService::jsonFieldString(const std::string& body, const char* key) const
{
	RapidJsonDocument document;
	if (!parseBody(body, document))
	{
		return "";
	}
	return document.getStringOrDefault(key, "");
}

int32_t CookApiService::jsonFieldInt(const std::string& body, const char* key, int32_t defaultValue) const
{
	RapidJsonDocument document;
	if (!parseBody(body, document))
	{
		return defaultValue;
	}
	return document.getIntOrDefault(key, defaultValue);
}

bool CookApiService::jsonFieldBool(const std::string& body, const char* key, bool defaultValue) const
{
	RapidJsonDocument document;
	if (!parseBody(body, document))
	{
		return defaultValue;
	}
	return document.getBoolOrDefault(key, defaultValue);
}

std::vector<std::string> CookApiService::parseRecipeIds(const std::string& body) const
{
	RapidJsonDocument document;
	if (!parseBody(body, document))
	{
		return std::vector<std::string>();
	}
	return document.getStringArrayOrEmpty("recipeIds");
}

bool CookApiService::authorize(const HttpRequest& request, std::string& userId, UserAccount& account, HttpResponse& response)
{
	std::string message;
	if (m_accountStore.validateToken(bearerToken(request), userId, account, message))
	{
		return true;
	}
	response = errorResponse(message.empty() ? "请先登录" : message, kHttpStatusUnauthorized, "UNAUTHORIZED");
	return false;
}

HttpResponse CookApiService::errorResponse(const std::string& message, int32_t statusCode, const std::string& errorCode) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", false);
	CookServerHelper::addString(document, "errorCode", errorCode.empty() ? "ERROR" : errorCode);
	CookServerHelper::addString(document, "message", message);
	return CookServerHelper::jsonResponse(document.toString(), statusCode);
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
	CookServerHelper::addString(document, "storageBackend", g_config.m_storageBackend);
	CookServerHelper::addBool(document, "storageEnabled", CookStorageService::instance().isEnabled());
	CookServerHelper::addBool(document, "redisMysqlStoreEnabled", CookStorageService::instance().isEnabled());
	CookServerHelper::addBool(document, "fileStoreEnabled", false);
	CookServerHelper::addString(document, "stateStore", "redis_mysql");
	return document.toString();
}

std::string CookApiService::buildRegisterJson(const std::string& body)
{
	RapidJsonDocument request;
	RapidJsonDocument document;
	document.setObject();
	if (!parseBody(body, request))
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "请求体不是合法JSON");
		return document.toString();
	}

	UserAccount account;
	std::string token;
	std::string message;
	int32_t expireInSeconds = 0;
	bool ok = m_accountStore.registerAccount(request.getStringOrDefault("account", ""),
		request.getStringOrDefault("password", ""),
		request.getStringOrDefault("nickname", ""),
		account,
		token,
		expireInSeconds,
		message);
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	if (ok)
	{
		CookServerHelper::addString(document, "token", token);
		CookServerHelper::addInt(document, "tokenExpireSeconds", expireInSeconds);
		document.addValue("account", CookServerHelper::accountToJson(account));
	}
	return document.toString();
}

std::string CookApiService::buildLoginJson(const std::string& body)
{
	RapidJsonDocument request;
	RapidJsonDocument document;
	document.setObject();
	if (!parseBody(body, request))
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "请求体不是合法JSON");
		return document.toString();
	}

	UserAccount account;
	std::string token;
	std::string message;
	int32_t expireInSeconds = 0;
	bool ok = m_accountStore.loginAccount(request.getStringOrDefault("account", ""),
		request.getStringOrDefault("password", ""),
		account,
		token,
		expireInSeconds,
		message);
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	if (ok)
	{
		CookServerHelper::addString(document, "token", token);
		CookServerHelper::addInt(document, "tokenExpireSeconds", expireInSeconds);
		document.addValue("account", CookServerHelper::accountToJson(account));
	}
	return document.toString();
}

std::string CookApiService::buildSessionJson(const UserAccount& account) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

bool CookApiService::recipeMatchesMenuCategory(const Recipe& recipe, const UserAccount& account, const std::string& categoryName) const
{
	if (categoryName == "最喜欢")
	{
		return account.m_favoriteRecipeIds.find(recipe.m_id) != account.m_favoriteRecipeIds.end();
	}
	if (categoryName == "购买区")
	{
		return AccountStore::isRecipeOwned(recipe, account) && !recipe.m_systemRecipe;
	}
	if (categoryName == "创作区")
	{
		return !recipe.m_systemRecipe;
	}
	return recipe.m_category == categoryName;
}

std::string CookApiService::buildRecipesJson(const std::string& userId)
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);

	UserAccount account = m_accountStore.getAccount(userId);
	std::vector<Recipe> recipes = m_accountStore.listRecipesForUser(userId);
	std::vector<Recipe> visibleRecipes;
	visibleRecipes.reserve(recipes.size());
	std::set<std::string> categorySet;
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		if (recipes[i].m_status == "published" || AccountStore::isRecipeOwned(recipes[i], account))
		{
			visibleRecipes.push_back(recipes[i]);
			categorySet.insert(recipes[i].m_category);
		}
	}

	std::vector<std::string> categoryNames;
	categoryNames.reserve(categorySet.size() + 3);
	categoryNames.push_back("最喜欢");
	categoryNames.push_back("购买区");
	categoryNames.push_back("创作区");
	for (std::set<std::string>::const_iterator it = categorySet.begin(); it != categorySet.end(); ++it)
	{
		categoryNames.push_back(*it);
	}

	RapidJsonValue categories;
	categories.setArray();
	categories.reserve(categoryNames.size());
	for (size_t i = 0; i < categoryNames.size(); ++i)
	{
		CookServerHelper::pushString(categories, categoryNames[i]);
	}
	document.addValue("categories", categories);

	RapidJsonValue list;
	list.setArray();
	list.reserve(visibleRecipes.size());
	for (size_t i = 0; i < visibleRecipes.size(); ++i)
	{
		list.pushValue(CookServerHelper::recipeSummaryToJson(visibleRecipes[i], &account));
	}
	document.addValue("recipes", list);
	RapidJsonValue categoryGroups;
	categoryGroups.setArray();
	categoryGroups.reserve(categoryNames.size());
	for (size_t categoryIndex = 0; categoryIndex < categoryNames.size(); ++categoryIndex)
	{
		RapidJsonValue group;
		group.setObject();
		CookServerHelper::addString(group, "name", categoryNames[categoryIndex]);
		RapidJsonValue groupRecipes;
		groupRecipes.setArray();
		for (size_t recipeIndex = 0; recipeIndex < visibleRecipes.size(); ++recipeIndex)
		{
			if (recipeMatchesMenuCategory(visibleRecipes[recipeIndex], account, categoryNames[categoryIndex]))
			{
				groupRecipes.pushValue(CookServerHelper::recipeSummaryToJson(visibleRecipes[recipeIndex], &account));
			}
		}
		CookServerHelper::addInt(group, "count", static_cast<int32_t>(groupRecipes.size()));
		group.addValue("recipes", groupRecipes);
		categoryGroups.pushValue(group);
	}
	document.addValue("categoryGroups", categoryGroups);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildRecipeDetailJson(const std::string& userId, const std::string& recipeId)
{
	RapidJsonDocument document;
	document.setObject();
	UserAccount account = m_accountStore.getAccount(userId);
	Recipe recipe;
	std::string message;
	bool ok = m_accountStore.getRecipeForUser(userId, recipeId, false, recipe, message);
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", ok ? "ok" : message);
	if (ok)
	{
		document.addValue("recipe", CookServerHelper::recipeToJson(recipe, &account));
	}
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildPersonalizationDetailJson(const std::string& userId, const std::string& recipeId)
{
	RapidJsonDocument document;
	document.setObject();
	UserAccount account = m_accountStore.getAccount(userId);
	Recipe recipe;
	PersonalizationConfig config;
	bool hasConfig = false;
	std::string message;
	bool ok = m_accountStore.getPersonalizationDetail(userId, recipeId, recipe, config, hasConfig, message);
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", ok ? "ok" : message);
	if (ok)
	{
		document.addValue("recipe", CookServerHelper::recipeToJson(recipe, &account));
		document.addValue("personalization", personalizationConfigToJson(config, hasConfig));
	}
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildFeedJson(const HttpRequest& request, const std::string& userId)
{
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", true);

	UserAccount account = m_accountStore.getAccount(userId);
	std::vector<Recipe> recipes = m_accountStore.listRecipesForUser(userId);
	std::map<std::string, Recipe> recipesById;
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		recipesById[recipes[i].m_id] = recipes[i];
	}
	RapidJsonValue tabs;
	tabs.setArray();
	tabs.reserve(4);
	CookServerHelper::pushString(tabs, "推荐");
	CookServerHelper::pushString(tabs, "精选");
	CookServerHelper::pushString(tabs, "关注");
	CookServerHelper::pushString(tabs, "好友");
	document.addValue("tabs", tabs);

	std::set<std::string> excludeVideoIds;
	std::vector<std::string> excludeParts = CookServerHelper::split(CookHelper::queryValue(request, "excludeIds"), ',');
	for (size_t i = 0; i < excludeParts.size(); ++i)
	{
		std::string excludeVideoId = CookHelper::normalizeVideoId(excludeParts[i]);
		if (!excludeVideoId.empty())
		{
			excludeVideoIds.insert(excludeVideoId);
		}
	}
	int32_t requestCount = CookHelper::queryInt(request, "count", g_config.m_videoFeedDefaultCount);
	std::string mode = CookHelper::queryValue(request, "mode");
	std::string currentVideoId = CookHelper::queryValue(request, "currentVideoId");
	bool resetSeen = false;
	std::vector<VideoInfo> videos = m_accountStore.listFeedVideosForUser(userId, requestCount, excludeVideoIds, mode, currentVideoId, resetSeen);
	account = m_accountStore.getAccount(userId);

	RapidJsonValue feed;
	feed.setArray();
	feed.reserve(videos.size());
	for (size_t i = 0; i < videos.size(); ++i)
	{
		feed.pushValue(feedVideoToJson(request, videos[i], account, recipesById, static_cast<int32_t>(i)));
	}
	document.addValue("feed", feed);
	CookServerHelper::addInt(document, "count", static_cast<int32_t>(videos.size()));
	CookServerHelper::addBool(document, "resetSeen", resetSeen);
	CookServerHelper::addInt(document, "videoSeenExpireDays", g_config.m_videoSeenExpireDays);
	CookServerHelper::addInt(document, "posterPrefetchCount", g_config.m_videoPosterPrefetchCount);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildFeedWatchJson(const std::string& userId, const std::string& body)
{
	int32_t watchedAtSeconds = 0;
	std::string message;
	bool ok = m_accountStore.markVideoWatched(userId, jsonFieldString(body, "videoId"), watchedAtSeconds, message);
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	CookServerHelper::addInt(document, "watchedAtSeconds", watchedAtSeconds);
	document.addValue("account", CookServerHelper::accountToJson(m_accountStore.getAccount(userId)));
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
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	return document.toString();
}

std::string CookApiService::buildJoinRecipeJson(const std::string& userId, const std::string& recipeId, const std::string& idempotencyKey)
{
	bool ok = false;
	bool alreadyOwned = false;
	std::string message;
	Recipe recipe;
	UserAccount account = m_accountStore.joinRecipe(userId, recipeId, idempotencyKey, &ok, &alreadyOwned, &message, &recipe);
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addBool(document, "alreadyOwned", alreadyOwned);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	if (!recipe.m_id.empty())
	{
		document.addValue("recipe", CookServerHelper::recipeToJson(recipe, &account));
	}
	return document.toString();
}

std::string CookApiService::buildFavoriteToggleJson(const std::string& userId, const std::string& targetType, const std::string& targetId)
{
	bool ok = false;
	bool favorite = false;
	std::string message;
	UserAccount account = m_accountStore.toggleFavorite(userId, targetType, targetId, &ok, &favorite, &message);
	RapidJsonDocument document;
	document.setObject();
	document.addBool("ok", ok);
	CookServerHelper::addBool(document, "favorite", favorite);
	CookServerHelper::addString(document, "message", message);
	document.addValue("account", CookServerHelper::accountToJson(account));
	if ((targetType.empty() || targetType == "recipe") && !targetId.empty())
	{
		Recipe recipe;
		std::string recipeMessage;
		if (m_accountStore.getRecipeForUser(userId, targetId, false, recipe, recipeMessage))
		{
			document.addValue("recipe", CookServerHelper::recipeToJson(recipe, &account));
		}
	}
	return document.toString();
}

std::string CookApiService::buildPlanJson(const std::vector<std::string>& ids, const std::string& userId, bool includePlanId, const std::string& planId, bool summaryOnly, bool includeTimeBlocks, bool lite)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	LOGINFO("CookApiService plan build begin userId=%s recipeCount=%d includePlanId=%d summaryOnly=%d includeTimeBlocks=%d lite=%d",
	        userId.c_str(),
	        static_cast<int32_t>(ids.size()),
	        includePlanId ? 1 : 0,
	        summaryOnly ? 1 : 0,
	        includeTimeBlocks ? 1 : 0,
	        lite ? 1 : 0);

	RapidJsonDocument document;
	document.setObject();
	if (ids.empty())
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "recipeIds is required");
		LOGWARNING("CookApiService plan build failed userId=%s reason=empty_recipe_ids summaryOnly=%d includeTimeBlocks=%d lite=%d costMs=%d",
		           userId.c_str(),
		           summaryOnly ? 1 : 0,
		           includeTimeBlocks ? 1 : 0,
		           lite ? 1 : 0,
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
		return document.toString();
	}

	std::vector<Recipe> recipes;
	recipes.reserve(ids.size());
	for (size_t i = 0; i < ids.size(); ++i)
	{
		Recipe recipe;
		std::string message;
		if (!m_accountStore.getRecipeForUser(userId, ids[i], true, recipe, message))
		{
			document.addBool("ok", false);
			CookServerHelper::addString(document, "message", message);
			LOGWARNING("CookApiService plan build failed userId=%s recipeId=%s message=%s summaryOnly=%d includeTimeBlocks=%d lite=%d costMs=%d",
			           userId.c_str(),
			           ids[i].c_str(),
			           message.c_str(),
			           summaryOnly ? 1 : 0,
			           includeTimeBlocks ? 1 : 0,
			           lite ? 1 : 0,
			           CSystem::GetHighTickCountMilliRunTime(beginTime));
			return document.toString();
		}
		recipes.push_back(recipe);
	}

	PlanResult plan = CookSchedule::buildPlanFromRecipes(recipes);
	std::string effectivePlanId = planId;
	if (includePlanId && effectivePlanId.empty())
	{
		effectivePlanId = "plan_" + CStringManager::toStringInt32(static_cast<int32_t>(::time(nullptr)));
	}
	RapidJsonValue root;
	if (summaryOnly)
	{
		root = CookServerHelper::planSummaryToJson(plan, includePlanId, includeTimeBlocks);
	}
	else
	{
		UserAccount account = m_accountStore.getAccount(userId);
		root = CookServerHelper::planToJson(plan, account, includePlanId, effectivePlanId, lite);
		if (!lite)
		{
			root.addValue("account", CookServerHelper::accountToJson(account));
		}
		RapidJsonValue clientConfig;
		clientConfig.setObject();
		CookServerHelper::addInt(clientConfig, "voiceInitialRequiredCount", g_config.m_voiceInitialRequiredCount);
		CookServerHelper::addInt(clientConfig, "voiceInitialPromptGroupCount", g_config.m_voiceInitialPromptGroupCount);
		CookServerHelper::addInt(clientConfig, "voiceClientConcurrency", g_config.m_voiceClientConcurrency);
		CookServerHelper::addInt(clientConfig, "voiceFetchWaitMilliseconds", g_config.m_voiceFetchWaitMilliseconds);
		CookServerHelper::addInt(clientConfig, "voiceFetchMaxAttempts", g_config.m_voiceFetchMaxAttempts);
		root.addValue("clientConfig", clientConfig);
	}
	std::string responseBody = root.toString();
	LOGINFO("CookApiService plan build success userId=%s recipeCount=%d totalSeconds=%d activeSeconds=%d taskCount=%d includePlanId=%d summaryOnly=%d includeTimeBlocks=%d lite=%d responseBytes=%d costMs=%d",
	        userId.c_str(),
	        static_cast<int32_t>(ids.size()),
	        plan.m_totalSeconds,
	        plan.m_activeSeconds,
	        static_cast<int32_t>(plan.m_timeline.size()),
	        includePlanId ? 1 : 0,
	        summaryOnly ? 1 : 0,
	        includeTimeBlocks ? 1 : 0,
	        lite ? 1 : 0,
	        static_cast<int32_t>(responseBody.size()),
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	return responseBody;
}

std::string CookApiService::buildRequestBaseUrl(const HttpRequest& request) const
{
	std::string host = headerValue(request, "Host");
	if (host.empty())
	{
		host = g_config.m_serverPublicIp + ":" + CStringManager::toStringInt32(static_cast<int32_t>(g_config.m_httpPort));
	}
	return std::string(request.m_isSsl ? "https://" : "http://") + host;
}

std::string CookApiService::buildVideoFileUrl(const HttpRequest& request, const std::string& videoId) const
{
	return buildRequestBaseUrl(request) + "/api/videos/file?videoId=" + CStringManager::UrlEncode(videoId);
}

std::string CookApiService::buildVideoPosterUrl(const HttpRequest& request, const std::string& videoId) const
{
	std::string apiPath = g_config.m_videoPosterApiPath.empty() ? "/api/videos/poster" : g_config.m_videoPosterApiPath;
	return buildRequestBaseUrl(request) + apiPath + "?videoId=" + CStringManager::UrlEncode(videoId);
}

bool CookApiService::parseVideoRange(const std::string& rangeHeader, int64_t fileSize, int64_t& startByte, int64_t& endByte) const
{
	std::string rangeText;
	for (size_t i = 0; i < rangeHeader.size(); ++i)
	{
		if (rangeHeader[i] != ' ' && rangeHeader[i] != '\t')
		{
			rangeText.push_back(rangeHeader[i]);
		}
	}
	std::string prefix = "bytes=";
	if (fileSize <= 0 || rangeText.find(prefix) != 0)
	{
		return false;
	}
	std::string value = rangeText.substr(prefix.size());
	size_t comma = value.find(',');
	if (comma != std::string::npos)
	{
		value = value.substr(0, comma);
	}
	size_t dash = value.find('-');
	if (dash == std::string::npos)
	{
		return false;
	}
	std::string startText = value.substr(0, dash);
	std::string endText = value.substr(dash + 1);
	if (startText.empty())
	{
		int64_t suffixBytes = CStringManager::atoi64(endText.c_str());
		if (suffixBytes <= 0)
		{
			return false;
		}
		startByte = suffixBytes >= fileSize ? 0 : fileSize - suffixBytes;
		endByte = fileSize - 1;
	}
	else
	{
		startByte = CStringManager::atoi64(startText.c_str());
		endByte = endText.empty() ? fileSize - 1 : CStringManager::atoi64(endText.c_str());
	}
	if (startByte < 0 || startByte >= fileSize || endByte < startByte)
	{
		return false;
	}
	if (endByte >= fileSize)
	{
		endByte = fileSize - 1;
	}
	if (g_config.m_videoStreamChunkBytes > 0 && endByte - startByte + 1 > g_config.m_videoStreamChunkBytes)
	{
		endByte = startByte + g_config.m_videoStreamChunkBytes - 1;
		if (endByte >= fileSize)
		{
			endByte = fileSize - 1;
		}
	}
	return true;
}

std::string CookApiService::buildVoicePrepareJson(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	RapidJsonDocument body;
	RapidJsonDocument document;
	document.setObject();
	if (!parseBody(request.m_body, body))
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "请求体不是合法JSON");
		return document.toString();
	}

	std::string planId = body.getStringOrDefault("planId", "");
	std::string gender = body.getStringOrDefault("gender", "female");
	std::vector<std::string> texts = body.getStringArrayOrEmpty("texts");
	int32_t requiredCount = body.getIntOrDefault("requiredCount", 0);
	bool includeRequiredAudio = body.getBoolOrDefault("includeRequiredAudio", false);
	int32_t requiredAudioWaitMilliseconds = body.getIntOrDefault("requiredAudioWaitMilliseconds", g_config.m_voiceFetchWaitMilliseconds);
	if (requiredAudioWaitMilliseconds < 0)
	{
		requiredAudioWaitMilliseconds = 0;
	}
	if (g_config.m_voiceFetchWaitMilliseconds > 0 && requiredAudioWaitMilliseconds > g_config.m_voiceFetchWaitMilliseconds)
	{
		requiredAudioWaitMilliseconds = g_config.m_voiceFetchWaitMilliseconds;
	}
	CookVoicePrepareResult prepare = CookVoiceService::instance().prepareVoices(planId, texts, gender, requiredCount);

	std::map<std::string, CookVoiceFetchResult> requiredAudioMap;
	int32_t requiredAudioReadyCount = 0;
	if (includeRequiredAudio && prepare.m_ok && requiredAudioWaitMilliseconds > 0)
	{
		std::chrono::high_resolution_clock::time_point audioBeginTime = CSystem::GetHighTickCount();
		for (size_t i = 0; i < prepare.m_requiredVoiceIds.size(); ++i)
		{
			int32_t elapsedMilliseconds = CSystem::GetHighTickCountMilliRunTime(audioBeginTime);
			int32_t remainMilliseconds = requiredAudioWaitMilliseconds - elapsedMilliseconds;
			if (remainMilliseconds < 0)
			{
				remainMilliseconds = 0;
			}
			CookVoiceFetchResult fetch = CookVoiceService::instance().fetchPreparedVoice(prepare.m_requiredVoiceIds[i], remainMilliseconds);
			if (fetch.m_ready)
			{
				++requiredAudioReadyCount;
			}
			requiredAudioMap[prepare.m_requiredVoiceIds[i]] = fetch;
		}
	}

	document.addBool("ok", prepare.m_ok);
	CookServerHelper::addString(document, "message", prepare.m_message);
	CookServerHelper::addString(document, "planId", prepare.m_planId);
	CookServerHelper::addString(document, "gender", prepare.m_gender);
	CookServerHelper::addInt(document, "requiredCount", prepare.m_requiredCount);
	CookServerHelper::addBool(document, "requiredAudioIncluded", includeRequiredAudio);
	CookServerHelper::addInt(document, "requiredAudioReadyCount", requiredAudioReadyCount);
	CookServerHelper::addInt(document, "requiredAudioWaitMilliseconds", requiredAudioWaitMilliseconds);
	CookServerHelper::addInt(document, "workerThreadCount", prepare.m_workerThreadCount);
	CookServerHelper::addInt(document, "clientConcurrency", g_config.m_voiceClientConcurrency);
	CookServerHelper::addInt(document, "fetchWaitMilliseconds", g_config.m_voiceFetchWaitMilliseconds);
	CookServerHelper::addInt(document, "fetchMaxAttempts", g_config.m_voiceFetchMaxAttempts);

	RapidJsonValue items;
	items.setArray();
	items.reserve(prepare.m_items.size());
	for (size_t i = 0; i < prepare.m_items.size(); ++i)
	{
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "voiceId", prepare.m_items[i].m_voiceId);
		CookServerHelper::addString(item, "text", prepare.m_items[i].m_text);
		CookServerHelper::addBool(item, "required", prepare.m_items[i].m_required);
		CookServerHelper::addInt(item, "order", prepare.m_items[i].m_order);
		std::map<std::string, CookVoiceFetchResult>::const_iterator fetchIt = requiredAudioMap.find(prepare.m_items[i].m_voiceId);
		CookServerHelper::addString(item, "status", fetchIt != requiredAudioMap.end() ? fetchIt->second.m_status : prepare.m_items[i].m_status);
		if (fetchIt != requiredAudioMap.end())
		{
			const CookVoiceFetchResult& fetch = fetchIt->second;
			CookServerHelper::addBool(item, "ready", fetch.m_ready);
			if (fetch.m_ready)
			{
				const CookVoiceTtsResult& result = fetch.m_tts;
				CookServerHelper::addString(item, "provider", result.m_provider.empty() ? g_config.m_voiceTtsProvider : result.m_provider);
				CookServerHelper::addString(item, "format", result.m_format);
				CookServerHelper::addString(item, "contentType", result.m_contentType);
				CookServerHelper::addBool(item, "cacheHit", result.m_cacheHit);
				CookServerHelper::addInt(item, "audioBytes", result.m_audioBytes);
				CookServerHelper::addString(item, "audioKey", result.m_audioKey);
				CookServerHelper::addString(item, "audioUrl", buildRequestBaseUrl(request) + g_config.m_voiceAudioApiPath + "?key=" + result.m_audioKey);
			}
		}
		items.pushValue(item);
	}
	document.addValue("items", items);

	RapidJsonValue requiredIds;
	requiredIds.setArray();
	requiredIds.reserve(prepare.m_requiredVoiceIds.size());
	for (size_t i = 0; i < prepare.m_requiredVoiceIds.size(); ++i)
	{
		CookServerHelper::pushString(requiredIds, prepare.m_requiredVoiceIds[i]);
	}
	document.addValue("requiredVoiceIds", requiredIds);

	RapidJsonValue backgroundIds;
	backgroundIds.setArray();
	backgroundIds.reserve(prepare.m_backgroundVoiceIds.size());
	for (size_t i = 0; i < prepare.m_backgroundVoiceIds.size(); ++i)
	{
		CookServerHelper::pushString(backgroundIds, prepare.m_backgroundVoiceIds[i]);
	}
	document.addValue("backgroundVoiceIds", backgroundIds);

	LOGINFO("CookApiService voice prepare planId=%s total=%d required=%d includeRequiredAudio=%d requiredAudioReady=%d requiredAudioWaitMs=%d costMs=%d",
	        prepare.m_planId.c_str(),
	        static_cast<int32_t>(prepare.m_items.size()),
	        static_cast<int32_t>(prepare.m_requiredVoiceIds.size()),
	        includeRequiredAudio ? 1 : 0,
	        requiredAudioReadyCount,
	        requiredAudioWaitMilliseconds,
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	return document.toString();
}

std::string CookApiService::buildVoiceFetchJson(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	RapidJsonDocument body;
	RapidJsonDocument document;
	document.setObject();
	if (!parseBody(request.m_body, body))
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "errorCode", "INVALID_JSON");
		CookServerHelper::addString(document, "message", "请求体不是合法JSON");
		return document.toString();
	}

	std::string voiceId = body.getStringOrDefault("voiceId", "");
	bool inlineAudio = body.getBoolOrDefault("inlineAudio", false);
	int32_t waitMilliseconds = body.getIntOrDefault("waitMilliseconds", g_config.m_voiceFetchWaitMilliseconds);
	if (waitMilliseconds < 0)
	{
		waitMilliseconds = 0;
	}
	if (g_config.m_voiceFetchWaitMilliseconds > 0 && waitMilliseconds > g_config.m_voiceFetchWaitMilliseconds)
	{
		waitMilliseconds = g_config.m_voiceFetchWaitMilliseconds;
	}
	CookVoiceFetchResult fetch = CookVoiceService::instance().fetchPreparedVoice(voiceId, waitMilliseconds);
	bool failed = fetch.m_status == "failed";
	document.addBool("ok", !failed);
	document.addBool("ready", fetch.m_ready);
	CookServerHelper::addString(document, "status", fetch.m_status);
	CookServerHelper::addString(document, "voiceId", fetch.m_voiceId);
	CookServerHelper::addString(document, "text", fetch.m_text);
	CookServerHelper::addString(document, "message", fetch.m_ready ? "ok" : fetch.m_message);
	if (!fetch.m_ready)
	{
		CookServerHelper::addString(document, "errorCode", failed ? "VOICE_FAILED" : "VOICE_PENDING");
		LOGINFO("CookApiService voice fetch pending voiceId=%s status=%s waitMs=%d costMs=%d",
		        voiceId.c_str(),
		        fetch.m_status.c_str(),
		        waitMilliseconds,
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
		return document.toString();
	}

	CookVoiceTtsResult result = fetch.m_tts;
	CookServerHelper::addString(document, "provider", result.m_provider.empty() ? g_config.m_voiceTtsProvider : result.m_provider);
	CookServerHelper::addString(document, "format", result.m_format);
	CookServerHelper::addString(document, "contentType", result.m_contentType);
	CookServerHelper::addBool(document, "cacheHit", result.m_cacheHit);
	CookServerHelper::addInt(document, "audioBytes", result.m_audioBytes);
	std::string audioUrl = buildRequestBaseUrl(request) + g_config.m_voiceAudioApiPath + "?key=" + result.m_audioKey;
	CookServerHelper::addString(document, "audioKey", result.m_audioKey);
	CookServerHelper::addString(document, "audioUrl", audioUrl);
	if (inlineAudio && g_config.m_voiceInlineAudioMaxBytes > 0 && result.m_audioBytes > 0 && result.m_audioBytes <= g_config.m_voiceInlineAudioMaxBytes)
	{
		std::string audioData = CSystem::readFile(result.m_filePath);
		if (!audioData.empty())
		{
			CookServerHelper::addString(document, "audioBase64", CookHelper::base64Encode(audioData));
			CookServerHelper::addBool(document, "audioInline", true);
		}
	}
	LOGINFO("CookApiService voice fetch ready voiceId=%s audioKey=%s inline=%d costMs=%d",
	        voiceId.c_str(),
	        result.m_audioKey.c_str(),
	        inlineAudio ? 1 : 0,
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	return document.toString();
}

std::string CookApiService::buildVoiceTtsJson(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	RapidJsonDocument body;
	RapidJsonDocument document;
	document.setObject();
	if (!parseBody(request.m_body, body))
	{
		document.addBool("ok", false);
		CookServerHelper::addString(document, "message", "请求体不是合法JSON");
		LOGWARNING("CookApiService voice tts failed reason=invalid_json bodyBytes=%d costMs=%d",
		           static_cast<int32_t>(request.m_body.size()),
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
		return document.toString();
	}

	std::string text = body.getStringOrDefault("text", "");
	std::string gender = body.getStringOrDefault("gender", "female");
	bool inlineAudio = body.getBoolOrDefault("inlineAudio", false);
	LOGINFO("CookApiService voice tts begin gender=%s textBytes=%d bodyBytes=%d",
	        gender.c_str(),
	        static_cast<int32_t>(text.size()),
	        static_cast<int32_t>(request.m_body.size()));
	CookVoiceTtsResult result = CookVoiceService::instance().synthesize(text, gender);
	document.addBool("ok", result.m_ok);
	CookServerHelper::addString(document, "message", result.m_message);
	CookServerHelper::addString(document, "provider", result.m_provider.empty() ? g_config.m_voiceTtsProvider : result.m_provider);
	CookServerHelper::addString(document, "format", result.m_format);
	CookServerHelper::addString(document, "contentType", result.m_contentType);
	CookServerHelper::addBool(document, "cacheHit", result.m_cacheHit);
	CookServerHelper::addInt(document, "audioBytes", result.m_audioBytes);
	if (result.m_ok)
	{
		std::string audioUrl = buildRequestBaseUrl(request) + g_config.m_voiceAudioApiPath + "?key=" + result.m_audioKey;
		CookServerHelper::addString(document, "audioKey", result.m_audioKey);
		CookServerHelper::addString(document, "audioUrl", audioUrl);
		if (inlineAudio && g_config.m_voiceInlineAudioMaxBytes > 0 && result.m_audioBytes > 0 && result.m_audioBytes <= g_config.m_voiceInlineAudioMaxBytes)
		{
			std::string audioData = CSystem::readFile(result.m_filePath);
			if (!audioData.empty())
			{
				CookServerHelper::addString(document, "audioBase64", CookHelper::base64Encode(audioData));
				CookServerHelper::addBool(document, "audioInline", true);
				LOGINFO("CookApiService voice tts inline audio audioBytes=%d audioKey=%s",
				        result.m_audioBytes,
				        result.m_audioKey.c_str());
			}
		}
	}
	if (result.m_ok)
	{
		LOGINFO("CookApiService voice tts success provider=%s gender=%s cacheHit=%d audioBytes=%d audioKey=%s costMs=%d",
		        result.m_provider.c_str(),
		        gender.c_str(),
		        result.m_cacheHit ? 1 : 0,
		        result.m_audioBytes,
		        result.m_audioKey.c_str(),
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	else
	{
		LOGWARNING("CookApiService voice tts failed provider=%s gender=%s message=%s textBytes=%d costMs=%d",
		           result.m_provider.c_str(),
		           gender.c_str(),
		           result.m_message.c_str(),
		           static_cast<int32_t>(text.size()),
		           CSystem::GetHighTickCountMilliRunTime(beginTime));
	}
	return document.toString();
}

HttpResponse CookApiService::buildVoiceAudioResponse(const HttpRequest& request) const
{
	std::string filePath;
	std::string contentType;
	std::string format;
	if (!CookVoiceService::instance().resolveAudioFile(CookHelper::queryValue(request, "key"), filePath, contentType, format))
	{
		return errorResponse("voice audio not found", kHttpStatusNotFound, "VOICE_AUDIO_NOT_FOUND");
	}

	std::string body = CSystem::readFile(filePath);
	if (body.empty())
	{
		return errorResponse("voice audio not found", kHttpStatusNotFound, "VOICE_AUDIO_NOT_FOUND");
	}

	HttpResponse response = HttpResponse::text(body);
	response.m_contentType = contentType;
	response.setHeader("Cache-Control", "public, max-age=31536000, immutable");
	response.setHeader("Content-Disposition", "inline; filename=\"cook_voice." + format + "\"");
	return response;
}

HttpResponse CookApiService::buildVideoFileResponse(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	VideoInfo video;
	std::string filePath;
	std::string contentType;
	std::string videoId = CookHelper::queryValue(request, "videoId");
	if (!m_accountStore.getVideoFile(videoId, video, filePath, contentType))
	{
		LOGWARNING("CookApiService video file not found videoId=%s", videoId.c_str());
		return errorResponse("video file not found", kHttpStatusNotFound, "VIDEO_NOT_FOUND");
	}

	std::ifstream input(filePath.c_str(), std::ios::binary | std::ios::ate);
	if (!input)
	{
		LOGWARNING("CookApiService video file open failed videoId=%s filePath=%s", videoId.c_str(), filePath.c_str());
		return errorResponse("video file not found", kHttpStatusNotFound, "VIDEO_NOT_FOUND");
	}
	int64_t fileSize = static_cast<int64_t>(input.tellg());
	if (fileSize <= 0)
	{
		LOGWARNING("CookApiService video file empty videoId=%s filePath=%s", videoId.c_str(), filePath.c_str());
		return errorResponse("video file is empty", kHttpStatusNotFound, "VIDEO_EMPTY");
	}

	int64_t startByte = 0;
	int64_t endByte = fileSize - 1;
	std::string rangeHeader = headerValue(request, "Range");
	bool partial = false;
	if (!rangeHeader.empty())
	{
		if (!parseVideoRange(rangeHeader, fileSize, startByte, endByte))
		{
			HttpResponse response(416);
			response.m_statusText = "Range Not Satisfiable";
			response.m_contentType = contentType;
			response.setHeader("Accept-Ranges", "bytes");
			response.setHeader("Content-Range", "bytes */" + CStringManager::toStringInt64(fileSize));
			response.setHeader("Cache-Control", "public, max-age=3600");
			LOGWARNING("CookApiService video range invalid videoId=%s range=%s fileSize=%s",
			           videoId.c_str(),
			           rangeHeader.c_str(),
			           CStringManager::toStringInt64(fileSize).c_str());
			return response;
		}
		partial = true;
	}
	else if (g_config.m_videoStreamChunkBytes > 0 && fileSize > g_config.m_videoStreamChunkBytes)
	{
		endByte = g_config.m_videoStreamChunkBytes - 1;
		if (endByte >= fileSize)
		{
			endByte = fileSize - 1;
		}
		partial = true;
		LOGINFO("CookApiService video no range capped videoId=%s fileSize=%s chunkBytes=%s",
		        videoId.c_str(),
		        CStringManager::toStringInt64(fileSize).c_str(),
		        CStringManager::toStringInt64(g_config.m_videoStreamChunkBytes).c_str());
	}

	int64_t readBytes = endByte - startByte + 1;
	std::string body;
	try
	{
		body.resize(static_cast<size_t>(readBytes));
	}
	catch (...)
	{
		LOGERROR("CookApiService video response allocate failed videoId=%s bytes=%s",
		         videoId.c_str(),
		         CStringManager::toStringInt64(readBytes).c_str());
		return errorResponse("video response too large", kHttpStatusInternalServerError, "VIDEO_ALLOC_FAILED");
	}
	input.seekg(startByte, std::ios::beg);
	input.read(&body[0], static_cast<std::streamsize>(body.size()));
	std::streamsize readCount = input.gcount();
	if (readCount <= 0)
	{
		LOGWARNING("CookApiService video read failed videoId=%s filePath=%s start=%s end=%s",
		           videoId.c_str(),
		           filePath.c_str(),
		           CStringManager::toStringInt64(startByte).c_str(),
		           CStringManager::toStringInt64(endByte).c_str());
		return errorResponse("video read failed", kHttpStatusInternalServerError, "VIDEO_READ_FAILED");
	}
	if (static_cast<size_t>(readCount) < body.size())
	{
		body.resize(static_cast<size_t>(readCount));
		endByte = startByte + static_cast<int64_t>(body.size()) - 1;
	}

	HttpResponse response(partial ? 206 : kHttpStatusOk);
	if (partial)
	{
		response.m_statusText = "Partial Content";
	}
	response.m_contentType = contentType;
	response.m_body = body;
	response.setHeader("Accept-Ranges", "bytes");
	response.setHeader("Cache-Control", "public, max-age=3600");
	response.setHeader("Content-Disposition", "inline; filename=\"" + video.m_id + "." + CSystem::GetName(filePath, 2) + "\"");
	if (partial)
	{
		response.setHeader("Content-Range",
			"bytes " + CStringManager::toStringInt64(startByte) + "-" + CStringManager::toStringInt64(endByte) + "/" + CStringManager::toStringInt64(fileSize));
	}
	LOGINFO("CookApiService video response videoId=%s partial=%d start=%s end=%s fileSize=%s bodyBytes=%d contentType=%s costMs=%d",
	        videoId.c_str(),
	        partial ? 1 : 0,
	        CStringManager::toStringInt64(startByte).c_str(),
	        CStringManager::toStringInt64(endByte).c_str(),
	        CStringManager::toStringInt64(fileSize).c_str(),
	        static_cast<int32_t>(body.size()),
	        contentType.c_str(),
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	return response;
}

bool CookApiService::readVideoPosterCache(const std::string& videoId, std::string& body, std::string& contentType)
{
	std::string realVideoId = CookHelper::normalizeVideoId(videoId);
	if (realVideoId.empty())
	{
		return false;
	}
	std::lock_guard<std::mutex> lock(m_videoPosterCacheMutex);
	std::map<std::string, VideoPosterCacheEntry>::const_iterator it = m_videoPosterCache.find(realVideoId);
	if (it == m_videoPosterCache.end() || it->second.m_body.empty())
	{
		return false;
	}
	body = it->second.m_body;
	contentType = it->second.m_contentType.empty() ? "image/jpeg" : it->second.m_contentType;
	return true;
}

void CookApiService::writeVideoPosterCache(const std::string& videoId, const std::string& body, const std::string& contentType)
{
	std::string realVideoId = CookHelper::normalizeVideoId(videoId);
	if (realVideoId.empty() || body.empty() || g_config.m_videoPosterMemoryCacheMaxItems <= 0)
	{
		return;
	}

	std::lock_guard<std::mutex> lock(m_videoPosterCacheMutex);
	if (m_videoPosterCache.find(realVideoId) == m_videoPosterCache.end())
	{
		m_videoPosterCacheOrder.push_back(realVideoId);
	}

	VideoPosterCacheEntry entry;
	entry.m_body = body;
	entry.m_contentType = contentType.empty() ? "image/jpeg" : contentType;
	m_videoPosterCache[realVideoId] = entry;

	int32_t maxItems = g_config.m_videoPosterMemoryCacheMaxItems;
	while (static_cast<int32_t>(m_videoPosterCacheOrder.size()) > maxItems)
	{
		std::string eraseId = m_videoPosterCacheOrder.front();
		m_videoPosterCacheOrder.pop_front();
		m_videoPosterCache.erase(eraseId);
	}
	while (static_cast<int32_t>(m_videoPosterCache.size()) > maxItems)
	{
		m_videoPosterCache.erase(m_videoPosterCache.begin());
	}
}

HttpResponse CookApiService::buildVideoPosterResponse(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	if (!g_config.m_videoPosterGenerateEnabled || g_config.m_videoPosterCommandTemplate.empty())
	{
		return errorResponse("video poster disabled", kHttpStatusNotFound, "VIDEO_POSTER_DISABLED");
	}

	VideoInfo video;
	std::string videoFilePath;
	std::string videoContentType;
	std::string videoId = CookHelper::queryValue(request, "videoId");
	LOGINFO("CookApiService video poster request begin videoId=%s", videoId.c_str());
	std::string cachedBody;
	std::string cachedContentType;
	if (readVideoPosterCache(videoId, cachedBody, cachedContentType))
	{
		HttpResponse response = HttpResponse::text(cachedBody);
		response.m_contentType = cachedContentType;
		response.setHeader("Cache-Control", "public, max-age=86400");
		response.setHeader("Content-Disposition", "inline; filename=\"" + CookHelper::normalizeVideoId(videoId) + ".jpg\"");
		LOGINFO("CookApiService video poster cache hit videoId=%s bytes=%d costMs=%d",
		        CookHelper::normalizeVideoId(videoId).c_str(),
		        static_cast<int32_t>(cachedBody.size()),
		        CSystem::GetHighTickCountMilliRunTime(beginTime));
		return response;
	}

	if (!m_accountStore.getVideoFile(videoId, video, videoFilePath, videoContentType))
	{
		LOGWARNING("CookApiService video poster source not found videoId=%s", videoId.c_str());
		return errorResponse("video file not found", kHttpStatusNotFound, "VIDEO_NOT_FOUND");
	}

	std::string command = g_config.m_videoPosterCommandTemplate;
	CStringManager::Replace(command, "{videoFile}", CookHelper::quoteCommandArgument(videoFilePath));

	std::string output;
	int32_t commandResult = CSystem::SystemCommand(command, output, g_config.m_videoPosterCommandUseShell);
	if (commandResult != 0 || !CookHelper::isJpegImageData(output))
	{
		LOGWARNING("CookApiService video poster pipe failed videoId=%s result=%d command=%s outputBytes=%d outputSample=%s",
		           video.m_id.c_str(),
		           commandResult,
		           command.c_str(),
		           static_cast<int32_t>(output.size()),
		           CookHelper::shortenCommandOutput(output).c_str());
		return errorResponse("video poster generate failed", kHttpStatusNotFound, "VIDEO_POSTER_GENERATE_FAILED");
	}

	HttpResponse response = HttpResponse::text(output);
	response.m_contentType = "image/jpeg";
	response.setHeader("Cache-Control", "public, max-age=86400");
	response.setHeader("Content-Disposition", "inline; filename=\"" + video.m_id + ".jpg\"");
	writeVideoPosterCache(video.m_id, output, response.m_contentType);
	LOGINFO("CookApiService video poster response videoId=%s bytes=%d costMs=%d",
	        video.m_id.c_str(),
	        static_cast<int32_t>(output.size()),
	        CSystem::GetHighTickCountMilliRunTime(beginTime));
	return response;
}

bool CookApiService::parsePersonalizationConfig(const std::string& body,
                                                const std::string& userId,
                                                PersonalizationConfig& config,
                                                std::string& message) const
{
	RapidJsonDocument document;
	if (!parseBody(body, document))
	{
		message = "请求体不是合法JSON";
		return false;
	}
	config = PersonalizationConfig();
	config.m_userId = userId;
	config.m_recipeId = document.getStringOrDefault("recipeId", "");
	if (config.m_recipeId.empty())
	{
		message = "recipeId is required";
		return false;
	}

	std::vector<std::string> disabled = document.getStringArrayOrEmpty("disabledTaskIds");
	for (size_t i = 0; i < disabled.size(); ++i)
	{
		if (!disabled[i].empty())
		{
			config.m_disabledTaskIds.insert(disabled[i]);
		}
	}
	std::vector<RapidJsonValue> overrides = document.getArrayValueOrEmpty("durationOverrides");
	for (size_t i = 0; i < overrides.size(); ++i)
	{
		std::string taskId = overrides[i].getStringOrDefault("taskId", "");
		int32_t durationSeconds = overrides[i].getIntOrDefault("durationSeconds", -1);
		if (!taskId.empty() && durationSeconds >= 0)
		{
			config.m_durationOverrides[taskId] = durationSeconds;
		}
	}
	std::vector<RapidJsonValue> addedTasks = document.getArrayValueOrEmpty("addedTasks");
	for (size_t i = 0; i < addedTasks.size(); ++i)
	{
		CookTask task;
		task.m_shortId = addedTasks[i].getStringOrDefault("id", addedTasks[i].getStringOrDefault("shortId", ""));
		task.m_recipeId = config.m_recipeId;
		task.m_id = config.m_recipeId + "." + (task.m_shortId.empty() ? ("added_" + CStringManager::toStringInt32(static_cast<int32_t>(i))) : task.m_shortId);
		task.m_title = addedTasks[i].getStringOrDefault("title", "");
		task.m_detail = addedTasks[i].getStringOrDefault("detail", "");
		task.m_resource = addedTasks[i].getStringOrDefault("resource", "");
		task.m_voiceText = addedTasks[i].getStringOrDefault("voiceText", "");
		task.m_safetyLevel = addedTasks[i].getStringOrDefault("safetyLevel", "normal");
		task.m_backgroundWaitMode = addedTasks[i].getStringOrDefault("backgroundWaitMode", "");
		task.m_durationSeconds = addedTasks[i].getIntOrDefault("durationSeconds", 0);
		std::string taskMode = addedTasks[i].getStringOrDefault("taskMode", "");
		if (taskMode == "foreground")
		{
			task.m_active = true;
		}
		else if (taskMode == "background")
		{
			task.m_active = false;
		}
		else
		{
			task.m_active = addedTasks[i].getBoolOrDefault("active", false);
		}
		task.m_manualSkippable = addedTasks[i].getBoolOrDefault("manualSkippable", task.m_active);
		task.m_canPause = addedTasks[i].getBoolOrDefault("canPause", false);
		task.m_continuesDuringPause = addedTasks[i].getBoolOrDefault("continuesDuringPause", false);
		task.m_canLeaveKitchen = addedTasks[i].getBoolOrDefault("canLeaveKitchen", false);
		if (task.m_active)
		{
			task.m_backgroundWaitMode.clear();
		}
		else if (task.m_backgroundWaitMode != "free" && task.m_backgroundWaitMode != "watch")
		{
			task.m_backgroundWaitMode = task.m_canLeaveKitchen && task.m_safetyLevel != "attention" && task.m_safetyLevel != "danger" ? "free" : "watch";
		}
		if (!task.m_active)
		{
			task.m_canLeaveKitchen = task.m_backgroundWaitMode == "free";
		}
		std::vector<std::string> dependencies = addedTasks[i].getStringArrayOrEmpty("dependencies");
		for (size_t j = 0; j < dependencies.size(); ++j)
		{
			if (dependencies[j].empty())
			{
				continue;
			}
			std::string prefix = config.m_recipeId + ".";
			task.m_dependencies.push_back(dependencies[j].find(prefix) == 0 ? dependencies[j] : prefix + dependencies[j]);
		}
		if (!task.m_title.empty())
		{
			if (task.m_durationSeconds < 0)
			{
				message = "addedTask durationSeconds is invalid";
				return false;
			}
			if (task.m_voiceText.empty())
			{
				task.m_voiceText = task.m_title + "。" + task.m_detail;
			}
			config.m_addedTasks.push_back(task);
		}
	}
	return true;
}

RapidJsonValue CookApiService::personalizationTaskToJson(const CookTask& task) const
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addString(object, "id", task.m_shortId.empty() ? task.m_id : task.m_shortId);
	CookServerHelper::addString(object, "shortId", task.m_shortId);
	CookServerHelper::addString(object, "recipeId", task.m_recipeId);
	CookServerHelper::addString(object, "title", task.m_title);
	CookServerHelper::addString(object, "detail", task.m_detail);
	CookServerHelper::addString(object, "resource", task.m_resource);
	CookServerHelper::addString(object, "voiceText", task.m_voiceText);
	CookServerHelper::addString(object, "safetyLevel", task.m_safetyLevel);
	CookServerHelper::addString(object, "backgroundWaitMode", task.m_backgroundWaitMode);
	CookServerHelper::addInt(object, "durationSeconds", task.m_durationSeconds);
	CookServerHelper::addBool(object, "active", task.m_active);
	CookServerHelper::addString(object, "taskMode", task.m_active ? "foreground" : "background");
	CookServerHelper::addBool(object, "manualSkippable", task.m_manualSkippable);
	CookServerHelper::addBool(object, "canPause", task.m_canPause);
	CookServerHelper::addBool(object, "continuesDuringPause", task.m_continuesDuringPause);
	CookServerHelper::addBool(object, "canLeaveKitchen", task.m_canLeaveKitchen);
	CookServerHelper::addStringArray(object, "dependencies", task.m_dependencies);
	return object;
}

RapidJsonValue CookApiService::personalizationConfigToJson(const PersonalizationConfig& config, bool hasConfig) const
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addBool(object, "hasConfig", hasConfig);
	CookServerHelper::addString(object, "schemaVersion", "1.0");
	CookServerHelper::addString(object, "recipeId", config.m_recipeId);

	RapidJsonValue disabled;
	disabled.setArray();
	disabled.reserve(config.m_disabledTaskIds.size());
	for (std::set<std::string>::const_iterator it = config.m_disabledTaskIds.begin(); it != config.m_disabledTaskIds.end(); ++it)
	{
		CookServerHelper::pushString(disabled, *it);
	}
	object.addValue("disabledTaskIds", disabled);

	RapidJsonValue overrides;
	overrides.setArray();
	overrides.reserve(config.m_durationOverrides.size());
	for (std::map<std::string, int32_t>::const_iterator it = config.m_durationOverrides.begin(); it != config.m_durationOverrides.end(); ++it)
	{
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "taskId", it->first);
		CookServerHelper::addInt(item, "durationSeconds", it->second);
		overrides.pushValue(item);
	}
	object.addValue("durationOverrides", overrides);

	RapidJsonValue addedTasks;
	addedTasks.setArray();
	addedTasks.reserve(config.m_addedTasks.size());
	for (size_t i = 0; i < config.m_addedTasks.size(); ++i)
	{
		addedTasks.pushValue(personalizationTaskToJson(config.m_addedTasks[i]));
	}
	object.addValue("addedTasks", addedTasks);
	return object;
}

RapidJsonValue CookApiService::commentToJson(const CommentInfo& comment) const
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addString(object, "id", comment.m_id);
	CookServerHelper::addString(object, "targetType", comment.m_targetType);
	CookServerHelper::addString(object, "targetId", comment.m_targetId);
	CookServerHelper::addString(object, "userId", comment.m_userId);
	CookServerHelper::addString(object, "nickname", comment.m_nickname);
	CookServerHelper::addString(object, "content", comment.m_content);
	CookServerHelper::addInt(object, "score", comment.m_score);
	CookServerHelper::addInt(object, "createdAtSeconds", comment.m_createdAtSeconds);
	return object;
}

RapidJsonValue CookApiService::messageToJson(const MessageInfo& message) const
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addString(object, "id", message.m_id);
	CookServerHelper::addString(object, "fromUserId", message.m_fromUserId);
	CookServerHelper::addString(object, "toUserId", message.m_toUserId);
	CookServerHelper::addString(object, "type", message.m_type);
	CookServerHelper::addString(object, "title", message.m_title);
	CookServerHelper::addString(object, "text", message.m_text);
	CookServerHelper::addInt(object, "createdAtSeconds", message.m_createdAtSeconds);
	CookServerHelper::addBool(object, "read", message.m_read);
	return object;
}

RapidJsonValue CookApiService::videoToJson(const VideoInfo& video) const
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addString(object, "id", video.m_id);
	CookServerHelper::addString(object, "ownerUserId", video.m_ownerUserId);
	CookServerHelper::addString(object, "mediaType", video.m_mediaType);
	CookServerHelper::addString(object, "filePath", video.m_filePath);
	CookServerHelper::addString(object, "caption", video.m_caption);
	CookServerHelper::addString(object, "status", video.m_status);
	CookServerHelper::addInt(object, "createdAtSeconds", video.m_createdAtSeconds);
	CookServerHelper::addStringArray(object, "recipeIds", video.m_recipeIds);
	return object;
}

RapidJsonValue CookApiService::feedVideoToJson(const HttpRequest& request,
                                               const VideoInfo& video,
                                               const UserAccount& account,
                                               const std::map<std::string, Recipe>& recipesById,
                                               int32_t index)
{
	RapidJsonValue item;
	item.setObject();
	Recipe recipe;
	bool hasRecipe = false;
	for (size_t i = 0; i < video.m_recipeIds.size(); ++i)
	{
		std::map<std::string, Recipe>::const_iterator recipeIt = recipesById.find(video.m_recipeIds[i]);
		if (recipeIt != recipesById.end())
		{
			recipe = recipeIt->second;
			hasRecipe = true;
			break;
		}
	}

	std::string title = hasRecipe ? recipe.m_title : video.m_caption;
	if (title.empty())
	{
		title = video.m_id;
	}
	std::string description = video.m_caption.empty() && hasRecipe ? recipe.m_subtitle : video.m_caption;
	std::string author = hasRecipe ? recipe.m_author : (video.m_ownerUserId.empty() ? "厨友" : video.m_ownerUserId);
	std::string authorUserId = hasRecipe ? recipe.m_authorUserId : video.m_ownerUserId;
	std::string coverColor = hasRecipe ? recipe.m_coverColor : "#17211d";
	std::string targetKey = "video:" + video.m_id;

	CookServerHelper::addString(item, "id", video.m_id);
	CookServerHelper::addString(item, "videoId", video.m_id);
	CookServerHelper::addString(item, "mediaType", video.m_mediaType);
	CookServerHelper::addString(item, "videoUrl", buildVideoFileUrl(request, video.m_id));
	CookServerHelper::addString(item, "posterUrl", buildVideoPosterUrl(request, video.m_id));
	CookServerHelper::addString(item, "title", title);
	CookServerHelper::addString(item, "description", description);
	CookServerHelper::addString(item, "author", author);
	CookServerHelper::addString(item, "authorUserId", authorUserId);
	CookServerHelper::addString(item, "coverColor", coverColor);
	CookServerHelper::addString(item, "priceType", hasRecipe ? recipe.m_priceType : "free");
	CookServerHelper::addInt(item, "priceCoins", hasRecipe ? recipe.m_priceCoins : 0);
	CookServerHelper::addInt(item, "priceAmount", hasRecipe ? recipe.m_priceAmount : 0);
	CookServerHelper::addBool(item, "owned", hasRecipe ? AccountStore::isRecipeOwned(recipe, account) : false);
	CookServerHelper::addBool(item, "favorite", account.m_favoriteTargetIds.find(targetKey) != account.m_favoriteTargetIds.end());
	CookServerHelper::addBool(item, "liked", account.m_likedTargetIds.find(targetKey) != account.m_likedTargetIds.end());
	CookServerHelper::addBool(item, "spoonVisible", hasRecipe);
	CookServerHelper::addBool(item, "canPlayVideo", true);
	CookServerHelper::addString(item, "sourceType", video.m_ownerUserId == "system" ? "resource" : "uploaded");
	CookServerHelper::addInt(item, "likes", m_accountStore.countTargetLikes("video", video.m_id));
	CookServerHelper::addInt(item, "comments", m_accountStore.countTargetComments("video", video.m_id));
	CookServerHelper::addInt(item, "rankIndex", index);

	RapidJsonValue recipeIds;
	recipeIds.setArray();
	recipeIds.reserve(video.m_recipeIds.size());
	for (size_t i = 0; i < video.m_recipeIds.size(); ++i)
	{
		CookServerHelper::pushString(recipeIds, video.m_recipeIds[i]);
	}
	item.addValue("recipeIds", recipeIds);
	return item;
}

HttpResponse CookApiService::handle(const HttpRequest& request)
{
	std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
	std::string clientRunId = headerValue(request, "X-Cook-Run-Id");
	std::string clientRequestId = headerValue(request, "X-Cook-Request-Id");
	std::string clientAttempt = headerValue(request, "X-Cook-Attempt");
	std::string clientTraceId = headerValue(request, "X-Cook-Trace-Id");
	std::string clientBeginMs = headerValue(request, "X-Cook-Request-Begin-Ms");
	std::string clientQueueWaitMs = headerValue(request, "X-Cook-Queue-Wait-Ms");
	std::string clientPurpose = headerValue(request, "X-Cook-Purpose");
	LOGINFO("CookApiService request begin method=%s uri=%s remoteIp=%s remotePort=%d bodyBytes=%d clientTraceId=%s clientRunId=%s clientRequestId=%s clientAttempt=%s clientBeginMs=%s clientQueueWaitMs=%s clientPurpose=%s",
	        request.m_method.c_str(),
	        request.m_uri.c_str(),
	        request.m_remoteIp.c_str(),
	        request.m_remotePort,
	        static_cast<int32_t>(request.m_body.size()),
	        clientTraceId.c_str(),
	        clientRunId.c_str(),
	        clientRequestId.c_str(),
	        clientAttempt.c_str(),
	        clientBeginMs.c_str(),
	        clientQueueWaitMs.c_str(),
	        clientPurpose.c_str());

	HttpResponse response;
	if (request.m_method == "GET" && request.m_uri == "/api/health")
	{
		response = CookServerHelper::jsonResponse(buildHealthJson());
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/voice/audio")
	{
		response = buildVoiceAudioResponse(request);
	}
	else if (request.m_method == "GET" && request.m_uri == "/api/videos/file")
	{
		response = buildVideoFileResponse(request);
	}
	else if (request.m_method == "GET" && request.m_uri == (g_config.m_videoPosterApiPath.empty() ? "/api/videos/poster" : g_config.m_videoPosterApiPath))
	{
		response = buildVideoPosterResponse(request);
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/auth/register")
	{
		response = CookServerHelper::jsonResponse(buildRegisterJson(request.m_body));
	}
	else if (request.m_method == "POST" && request.m_uri == "/api/auth/login")
	{
		response = CookServerHelper::jsonResponse(buildLoginJson(request.m_body));
	}
	else
	{
		std::string userId;
		UserAccount account;
		HttpResponse authFailedResponse;
		bool authenticated = authorize(request, userId, account, authFailedResponse);
		bool legacyAllowed = false;
		if (!authenticated)
		{
			std::string legacyUserId = jsonFieldString(request.m_body, "userId");
			if (legacyUserId.empty())
			{
				legacyUserId = CookHelper::queryValue(request, "userId");
			}
			if (legacyUserId.empty() && request.m_uri == "/api/account/recharge")
			{
				legacyUserId = "demo_user";
			}
			if (!legacyUserId.empty() &&
				(request.m_uri == "/api/account/recharge" ||
				request.m_uri == "/api/recipes/purchase" ||
				request.m_uri == "/api/favorites/toggle" ||
				request.m_uri == "/api/plan/preview" ||
				request.m_uri == "/api/plan/start" ||
				request.m_uri == "/api/voice/prepare" ||
				request.m_uri == "/api/voice/fetch" ||
				request.m_uri == "/api/voice/tts" ||
				request.m_uri == "/api/recipes" ||
				request.m_uri == "/api/recipes/personalization/detail" ||
				request.m_uri == "/api/feed" ||
				request.m_uri == "/api/feed/watch" ||
				request.m_uri == "/api/account"))
			{
				userId = legacyUserId;
				account = m_accountStore.getAccount(userId);
				legacyAllowed = true;
			}
		}
		if (!authenticated && !legacyAllowed)
		{
			response = authFailedResponse;
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/auth/session")
		{
			response = CookServerHelper::jsonResponse(buildSessionJson(account));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/auth/logout")
		{
			m_accountStore.logout(bearerToken(request));
			response = CookServerHelper::jsonResponse("{\"ok\":true,\"message\":\"已退出登录\"}");
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/recipes")
		{
			response = CookServerHelper::jsonResponse(buildRecipesJson(userId));
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/recipes/detail")
		{
			response = CookServerHelper::jsonResponse(buildRecipeDetailJson(userId, CookHelper::queryValue(request, "recipeId")));
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/recipes/personalization/detail")
		{
			response = CookServerHelper::jsonResponse(buildPersonalizationDetailJson(userId, CookHelper::queryValue(request, "recipeId")));
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/feed")
		{
			response = CookServerHelper::jsonResponse(buildFeedJson(request, userId));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/feed/watch")
		{
			response = CookServerHelper::jsonResponse(buildFeedWatchJson(userId, request.m_body));
		}
		else if (request.m_method == "GET" && (request.m_uri == "/api/account" || request.m_uri == "/api/wallet"))
		{
			response = CookServerHelper::jsonResponse(buildAccountJson(userId));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/account/recharge")
		{
			response = CookServerHelper::jsonResponse(buildRechargeJson(userId, jsonFieldInt(request.m_body, "coins", 0)));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/wallet/checkin")
		{
			bool ok = false;
			bool alreadyCheckedIn = false;
			std::string message;
			UserAccount newAccount = m_accountStore.checkin(userId, &ok, &alreadyCheckedIn, &message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addBool(document, "alreadyCheckedIn", alreadyCheckedIn);
			CookServerHelper::addString(document, "message", message);
			document.addValue("account", CookServerHelper::accountToJson(newAccount));
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/wallet/ad/start")
		{
			std::string adToken;
			int32_t rewardCoins = 0;
			int32_t requiredSeconds = 0;
			UserAccount newAccount = m_accountStore.startAdReward(userId, adToken, rewardCoins, requiredSeconds);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", true);
			CookServerHelper::addString(document, "adToken", adToken);
			CookServerHelper::addInt(document, "rewardCoins", rewardCoins);
			CookServerHelper::addInt(document, "requiredSeconds", requiredSeconds);
			document.addValue("account", CookServerHelper::accountToJson(newAccount));
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/wallet/ad/finish")
		{
			bool ok = false;
			bool alreadyRewarded = false;
			std::string message;
			UserAccount newAccount = m_accountStore.finishAdReward(userId,
				jsonFieldString(request.m_body, "adToken"),
				jsonFieldInt(request.m_body, "debugElapsedSeconds", -1),
				&ok,
				&alreadyRewarded,
				&message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addBool(document, "alreadyRewarded", alreadyRewarded);
			CookServerHelper::addString(document, "message", message);
			document.addValue("account", CookServerHelper::accountToJson(newAccount));
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && (request.m_uri == "/api/recipes/join" || request.m_uri == "/api/recipes/purchase"))
		{
			std::string recipeId = jsonFieldString(request.m_body, "recipeId");
			std::string idempotencyKey = headerValue(request, "X-Idempotency-Key");
			if (idempotencyKey.empty())
			{
				idempotencyKey = jsonFieldString(request.m_body, "idempotencyKey");
			}
			response = CookServerHelper::jsonResponse(buildJoinRecipeJson(userId, recipeId, idempotencyKey));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/favorites/toggle")
		{
			std::string targetType = jsonFieldString(request.m_body, "targetType");
			std::string targetId = jsonFieldString(request.m_body, "targetId");
			if (targetId.empty())
			{
				targetType = "recipe";
				targetId = jsonFieldString(request.m_body, "recipeId");
			}
			response = CookServerHelper::jsonResponse(buildFavoriteToggleJson(userId, targetType, targetId));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/likes/toggle")
		{
			bool liked = false;
			bool ok = m_accountStore.toggleLike(userId, jsonFieldString(request.m_body, "targetType"), jsonFieldString(request.m_body, "targetId"), liked);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addBool(document, "liked", liked);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/follows/toggle")
		{
			bool following = false;
			std::string message;
			bool ok = m_accountStore.toggleFollow(userId, jsonFieldString(request.m_body, "targetUserId"), following, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addBool(document, "following", following);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/comments/list")
		{
			std::vector<CommentInfo> comments = m_accountStore.listComments(CookHelper::queryValue(request, "targetType"), CookHelper::queryValue(request, "targetId"));
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", true);
			RapidJsonValue list;
			list.setArray();
			list.reserve(comments.size());
			for (size_t i = 0; i < comments.size(); ++i)
			{
				list.pushValue(commentToJson(comments[i]));
			}
			document.addValue("comments", list);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/comments/create")
		{
			CommentInfo comment;
			std::string message;
			bool ok = m_accountStore.createComment(userId,
				jsonFieldString(request.m_body, "targetType"),
				jsonFieldString(request.m_body, "targetId"),
				jsonFieldString(request.m_body, "content"),
				jsonFieldInt(request.m_body, "score", 0),
				comment,
				message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			if (ok)
			{
				document.addValue("comment", commentToJson(comment));
			}
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/messages/list")
		{
			std::vector<MessageInfo> messages = m_accountStore.listMessages(userId);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", true);
			RapidJsonValue list;
			list.setArray();
			list.reserve(messages.size());
			for (size_t i = 0; i < messages.size(); ++i)
			{
				list.pushValue(messageToJson(messages[i]));
			}
			document.addValue("messages", list);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/videos/upload")
		{
			VideoInfo video;
			std::string message;
			bool ok = m_accountStore.uploadVideo(userId,
				jsonFieldString(request.m_body, "mediaType"),
				jsonFieldString(request.m_body, "filePath"),
				jsonFieldInt(request.m_body, "fileSizeBytes", 0),
				video,
				message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			if (ok)
			{
				document.addValue("video", videoToJson(video));
			}
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && (request.m_uri == "/api/videos/publish" || request.m_uri == "/api/videos/attach"))
		{
			VideoInfo video;
			std::string message;
			bool ok = m_accountStore.publishVideo(userId,
				jsonFieldString(request.m_body, "videoId"),
				jsonFieldString(request.m_body, "caption"),
				parseRecipeIds(request.m_body),
				video,
				message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			if (ok)
			{
				document.addValue("video", videoToJson(video));
			}
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && (request.m_uri == "/api/recipes/custom/save" || request.m_uri == "/api/recipes/custom/publish"))
		{
			std::string recipeJson = jsonFieldString(request.m_body, "recipeJson");
			if (recipeJson.empty() && request.m_body.find("\"schemaVersion\"") != std::string::npos)
			{
				recipeJson = request.m_body;
			}
			Recipe sourceRecipe;
			std::string parseMessage;
			bool parseOk = CookRecipeConfig::loadRecipeFromJson(recipeJson, sourceRecipe, &parseMessage);
			Recipe savedRecipe;
			std::string message;
			bool ok = parseOk && m_accountStore.saveCustomRecipe(userId, sourceRecipe, request.m_uri == "/api/recipes/custom/publish", savedRecipe, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", ok ? message : (parseOk ? message : parseMessage));
			if (ok)
			{
				UserAccount newAccount = m_accountStore.getAccount(userId);
				document.addValue("recipe", CookServerHelper::recipeToJson(savedRecipe, &newAccount));
				document.addValue("account", CookServerHelper::accountToJson(newAccount));
			}
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/recipes/personalization/save")
		{
			PersonalizationConfig config;
			std::string message;
			bool ok = parsePersonalizationConfig(request.m_body, userId, config, message) &&
				m_accountStore.savePersonalization(userId, config, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/recipes/personalization/sync-times")
		{
			RapidJsonDocument body;
			std::string message;
			std::map<std::string, std::map<std::string, int32_t>> recipeDurationOverrides;
			bool parseOk = parseBody(request.m_body, body);
			if (!parseOk)
			{
				message = "请求体不是合法JSON";
			}
			else
			{
				std::vector<RapidJsonValue> items = body.getArrayValueOrEmpty("items");
				if (items.empty())
				{
					items = body.getArrayValueOrEmpty("recipes");
				}
				for (size_t i = 0; i < items.size(); ++i)
				{
					std::string recipeId = items[i].getStringOrDefault("recipeId", "");
					if (recipeId.empty())
					{
						continue;
					}
					std::vector<RapidJsonValue> overrides = items[i].getArrayValueOrEmpty("durationOverrides");
					for (size_t j = 0; j < overrides.size(); ++j)
					{
						std::string taskId = overrides[j].getStringOrDefault("taskId", "");
						int32_t durationSeconds = overrides[j].getIntOrDefault("durationSeconds", -1);
						if (!taskId.empty() && durationSeconds >= 0)
						{
							recipeDurationOverrides[recipeId][taskId] = durationSeconds;
						}
					}
				}
			}

			int32_t updatedRecipeCount = 0;
			int32_t updatedTaskCount = 0;
			bool ok = parseOk &&
				m_accountStore.syncPersonalizationDurations(userId, recipeDurationOverrides, updatedRecipeCount, updatedTaskCount, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addInt(document, "updatedRecipeCount", updatedRecipeCount);
			CookServerHelper::addInt(document, "updatedTaskCount", updatedTaskCount);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/recipes/personalization/reset")
		{
			std::string message;
			bool ok = m_accountStore.resetPersonalization(userId, jsonFieldString(request.m_body, "recipeId"), message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/recipes/personalization/share")
		{
			std::string shareId;
			std::string message;
			bool ok = m_accountStore.sharePersonalization(userId, jsonFieldString(request.m_body, "recipeId"), shareId, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "shareId", shareId);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/recipes/personalization/apply")
		{
			PersonalizationConfig config;
			std::string message;
			bool ok = m_accountStore.applySharedPersonalization(userId, jsonFieldString(request.m_body, "shareId"), config, message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "recipeId", config.m_recipeId);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/plan/preview")
		{
			response = CookServerHelper::jsonResponse(buildPlanJson(parseRecipeIds(request.m_body), userId, false, "", jsonFieldBool(request.m_body, "summaryOnly", false), jsonFieldBool(request.m_body, "includeTimeBlocks", false), false));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/plan/start")
		{
			response = CookServerHelper::jsonResponse(buildPlanJson(parseRecipeIds(request.m_body), userId, true, jsonFieldString(request.m_body, "planId"), false, false, jsonFieldBool(request.m_body, "lite", false)));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/voice/prepare")
		{
			response = CookServerHelper::jsonResponse(buildVoicePrepareJson(request));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/voice/fetch")
		{
			response = CookServerHelper::jsonResponse(buildVoiceFetchJson(request));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/voice/tts")
		{
			response = CookServerHelper::jsonResponse(buildVoiceTtsJson(request));
		}
		else if (request.m_method == "POST" && request.m_uri == "/api/cook/result/publish")
		{
			std::string message;
			bool ok = m_accountStore.recordCookResult(userId,
				jsonFieldString(request.m_body, "planId"),
				parseRecipeIds(request.m_body),
				jsonFieldString(request.m_body, "imagePath"),
				message);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", ok);
			CookServerHelper::addString(document, "message", message);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else if (request.m_method == "GET" && request.m_uri == "/api/cook/history")
		{
			std::vector<MessageInfo> messages = m_accountStore.listMessages(userId);
			RapidJsonDocument document;
			document.setObject();
			document.addBool("ok", true);
			RapidJsonValue list;
			list.setArray();
			for (size_t i = 0; i < messages.size(); ++i)
			{
				if (messages[i].m_type == "cook_result")
				{
					list.pushValue(messageToJson(messages[i]));
				}
			}
			document.addValue("history", list);
			response = CookServerHelper::jsonResponse(document.toString());
		}
		else
		{
			response = errorResponse("not found", kHttpStatusNotFound, "NOT_FOUND");
		}
	}

	if (!clientTraceId.empty())
	{
		response.setHeader("X-Cook-Trace-Id", clientTraceId);
	}
	LOGINFO("CookApiService request finish method=%s uri=%s status=%d costMs=%d responseBytes=%d clientTraceId=%s clientRunId=%s clientRequestId=%s clientAttempt=%s clientQueueWaitMs=%s clientPurpose=%s",
	        request.m_method.c_str(),
	        request.m_uri.c_str(),
	        response.m_statusCode,
	        CSystem::GetHighTickCountMilliRunTime(beginTime),
	        static_cast<int32_t>(response.m_body.size()),
	        clientTraceId.c_str(),
	        clientRunId.c_str(),
	        clientRequestId.c_str(),
	        clientAttempt.c_str(),
	        clientQueueWaitMs.c_str(),
	        clientPurpose.c_str());
	return response;
}
