#include "CookServerHelper.h"
#include "AccountStore.h"
#include "Config.h"
#include "CookCatalog.h"
#include "CookSchedule.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <ctime>
#include <map>

static const char* kCookServerRoutes[] =
{
	"/api/health",
	"/api/auth/register",
	"/api/auth/login",
	"/api/auth/session",
	"/api/auth/logout",
	"/api/account",
	"/api/account/recharge",
	"/api/wallet",
	"/api/wallet/checkin",
	"/api/wallet/ad/start",
	"/api/wallet/ad/finish",
	"/api/recipes",
	"/api/recipes/detail",
	"/api/recipes/join",
	"/api/recipes/purchase",
	"/api/recipes/custom/save",
	"/api/recipes/custom/publish",
	"/api/recipes/personalization/save",
	"/api/recipes/personalization/detail",
	"/api/recipes/personalization/sync-times",
	"/api/recipes/personalization/reset",
	"/api/recipes/personalization/share",
	"/api/recipes/personalization/apply",
	"/api/feed",
	"/api/feed/watch",
	"/api/favorites/toggle",
	"/api/likes/toggle",
	"/api/comments/list",
	"/api/comments/create",
	"/api/follows/toggle",
	"/api/messages/list",
	"/api/messages/send",
	"/api/videos/file",
	"/api/videos/poster",
	"/api/videos/upload",
	"/api/videos/publish",
	"/api/videos/attach",
	"/api/plan/preview",
	"/api/plan/start",
	"/api/voice/prepare",
	"/api/voice/fetch",
	"/api/voice/tts",
	"/api/voice/audio",
	"/api/cook/result/publish",
	"/api/cook/history"
};

std::vector<std::string> CookServerHelper::split(const std::string& text, char delimiter)
{
	std::vector<std::string> parts;
	std::vector<std::string> rawParts;
	CStringManager::split(rawParts, text, delimiter);
	for (size_t i = 0; i < rawParts.size(); ++i)
	{
		if (!rawParts[i].empty())
		{
			parts.push_back(rawParts[i]);
		}
	}
	return parts;
}

std::string CookServerHelper::joinSet(const std::set<std::string>& values, char delimiter)
{
	std::string result;
	for (std::set<std::string>::const_iterator it = values.begin(); it != values.end(); ++it)
	{
		if (!result.empty())
		{
			result.push_back(delimiter);
		}
		result += *it;
	}
	return result;
}

int32_t CookServerHelper::ceilMinutes(int32_t seconds)
{
	return (seconds + 59) / 60;
}

bool CookServerHelper::isUtf8Text(const std::string& text)
{
	const unsigned char* data = reinterpret_cast<const unsigned char*>(text.data());
	size_t index = 0;
	while (index < text.size())
	{
		unsigned char ch = data[index];
		if (ch <= 0x7F)
		{
			++index;
		}
		else if ((ch & 0xE0) == 0xC0)
		{
			if (index + 1 >= text.size() || (data[index + 1] & 0xC0) != 0x80 || ch < 0xC2)
			{
				return false;
			}
			index += 2;
		}
		else if ((ch & 0xF0) == 0xE0)
		{
			if (index + 2 >= text.size() || (data[index + 1] & 0xC0) != 0x80 || (data[index + 2] & 0xC0) != 0x80)
			{
				return false;
			}
			if ((ch == 0xE0 && data[index + 1] < 0xA0) || (ch == 0xED && data[index + 1] >= 0xA0))
			{
				return false;
			}
			index += 3;
		}
		else if ((ch & 0xF8) == 0xF0)
		{
			if (index + 3 >= text.size() || (data[index + 1] & 0xC0) != 0x80 || (data[index + 2] & 0xC0) != 0x80 || (data[index + 3] & 0xC0) != 0x80)
			{
				return false;
			}
			if ((ch == 0xF0 && data[index + 1] < 0x90) || (ch == 0xF4 && data[index + 1] >= 0x90) || ch > 0xF4)
			{
				return false;
			}
			index += 4;
		}
		else
		{
			return false;
		}
	}
	return true;
}

std::string CookServerHelper::textToJsonUtf8(const std::string& text)
{
	if (CookServerHelper::isUtf8Text(text))
	{
		return text;
	}
#ifdef _WIN32
	std::string utf8Text = CStringManager::AnsiToUtf8(text);
	return utf8Text.empty() && !text.empty() ? text : utf8Text;
#else
	return text;
#endif
}

std::string CookServerHelper::joinJsonUtf8Text(const std::string& first, const std::string& second)
{
	return CookServerHelper::textToJsonUtf8(first) + CookServerHelper::textToJsonUtf8(second);
}

std::string CookServerHelper::joinJsonUtf8Text(const std::string& first, const std::string& second, const std::string& third)
{
	return CookServerHelper::textToJsonUtf8(first) + CookServerHelper::textToJsonUtf8(second) + CookServerHelper::textToJsonUtf8(third);
}

void CookServerHelper::addString(RapidJsonValue& object, const char* key, const std::string& text)
{
	object.addString(key, CookServerHelper::textToJsonUtf8(text));
}

void CookServerHelper::addString(RapidJsonDocument& object, const char* key, const std::string& text)
{
	object.addString(key, CookServerHelper::textToJsonUtf8(text));
}

void CookServerHelper::addInt(RapidJsonValue& object, const char* key, int32_t number)
{
	object.addInt(key, number);
}

void CookServerHelper::addInt(RapidJsonDocument& object, const char* key, int32_t number)
{
	object.addInt(key, number);
}

void CookServerHelper::addBool(RapidJsonValue& object, const char* key, bool value)
{
	object.addBool(key, value);
}

void CookServerHelper::addBool(RapidJsonDocument& object, const char* key, bool value)
{
	object.addBool(key, value);
}

void CookServerHelper::pushString(RapidJsonValue& array, const std::string& text)
{
	array.pushString(CookServerHelper::textToJsonUtf8(text));
}

void CookServerHelper::addStringArray(RapidJsonValue& object, const char* key, const std::vector<std::string>& values)
{
	RapidJsonValue array;
	array.setArray();
	array.reserve(values.size());
	for (size_t i = 0; i < values.size(); ++i)
	{
		CookServerHelper::pushString(array, values[i]);
	}
	object.addValue(key, array);
}

RapidJsonValue CookServerHelper::accountToJson(const UserAccount& account)
{
	RapidJsonValue object;
	object.setObject();
	CookServerHelper::addString(object, "userId", account.m_userId);
	CookServerHelper::addString(object, "account", account.m_account);
	CookServerHelper::addString(object, "nickname", account.m_nickname);
	CookServerHelper::addInt(object, "coins", account.m_coins);
	CookServerHelper::addInt(object, "yuanbao", account.m_yuanbao);
	CookServerHelper::addString(object, "lastCheckinDate", account.m_lastCheckinDate);
	CookServerHelper::addBool(object, "personalizationRecordEnabled", account.m_personalizationRecordEnabled);

	RapidJsonValue owned;
	owned.setArray();
	owned.reserve(account.m_ownedRecipeIds.size());
	for (std::set<std::string>::const_iterator it = account.m_ownedRecipeIds.begin(); it != account.m_ownedRecipeIds.end(); ++it)
	{
		CookServerHelper::pushString(owned, *it);
	}
	object.addValue("ownedRecipeIds", owned);

	RapidJsonValue purchased;
	purchased.setArray();
	purchased.reserve(account.m_purchasedRecipeIds.size());
	for (std::set<std::string>::const_iterator it = account.m_purchasedRecipeIds.begin(); it != account.m_purchasedRecipeIds.end(); ++it)
	{
		CookServerHelper::pushString(purchased, *it);
	}
	object.addValue("purchasedRecipeIds", purchased);

	RapidJsonValue favorites;
	favorites.setArray();
	favorites.reserve(account.m_favoriteRecipeIds.size());
	for (std::set<std::string>::const_iterator it = account.m_favoriteRecipeIds.begin(); it != account.m_favoriteRecipeIds.end(); ++it)
	{
		CookServerHelper::pushString(favorites, *it);
	}
	object.addValue("favoriteRecipeIds", favorites);

	RapidJsonValue favoriteTargets;
	favoriteTargets.setArray();
	favoriteTargets.reserve(account.m_favoriteTargetIds.size());
	for (std::set<std::string>::const_iterator it = account.m_favoriteTargetIds.begin(); it != account.m_favoriteTargetIds.end(); ++it)
	{
		CookServerHelper::pushString(favoriteTargets, *it);
	}
	object.addValue("favoriteTargetIds", favoriteTargets);

	RapidJsonValue likedTargets;
	likedTargets.setArray();
	likedTargets.reserve(account.m_likedTargetIds.size());
	for (std::set<std::string>::const_iterator it = account.m_likedTargetIds.begin(); it != account.m_likedTargetIds.end(); ++it)
	{
		CookServerHelper::pushString(likedTargets, *it);
	}
	object.addValue("likedTargetIds", likedTargets);

	RapidJsonValue watchedVideos;
	watchedVideos.setArray();
	watchedVideos.reserve(account.m_watchedVideoAtSeconds.size());
	for (std::map<std::string, int32_t>::const_iterator it = account.m_watchedVideoAtSeconds.begin(); it != account.m_watchedVideoAtSeconds.end(); ++it)
	{
		CookServerHelper::pushString(watchedVideos, it->first);
	}
	object.addValue("watchedVideoIds", watchedVideos);

	RapidJsonValue customRecipes;
	customRecipes.setArray();
	customRecipes.reserve(account.m_customRecipeIds.size());
	for (std::set<std::string>::const_iterator it = account.m_customRecipeIds.begin(); it != account.m_customRecipeIds.end(); ++it)
	{
		CookServerHelper::pushString(customRecipes, *it);
	}
	object.addValue("customRecipeIds", customRecipes);

	RapidJsonValue following;
	following.setArray();
	following.reserve(account.m_followingUserIds.size());
	for (std::set<std::string>::const_iterator it = account.m_followingUserIds.begin(); it != account.m_followingUserIds.end(); ++it)
	{
		CookServerHelper::pushString(following, *it);
	}
	object.addValue("followingUserIds", following);
	return object;
}

RapidJsonValue CookServerHelper::recipeToJson(const Recipe& recipe, const UserAccount* account)
{
	RapidJsonValue object;
	object.setObject();
	bool owned = account == nullptr ? recipe.m_defaultOwned : AccountStore::isRecipeOwned(recipe, *account);
	bool accessible = owned || recipe.m_priceType == "free" || (recipe.m_priceAmount <= 0 && recipe.m_priceCoins <= 0);
	bool favorite = account != nullptr && account->m_favoriteRecipeIds.find(recipe.m_id) != account->m_favoriteRecipeIds.end();

	CookServerHelper::addString(object, "id", recipe.m_id);
	CookServerHelper::addString(object, "title", recipe.m_title);
	CookServerHelper::addString(object, "category", recipe.m_category);
	CookServerHelper::addString(object, "subtitle", recipe.m_subtitle);
	CookServerHelper::addString(object, "difficulty", recipe.m_difficulty);
	CookServerHelper::addString(object, "coverColor", recipe.m_coverColor);
	CookServerHelper::addString(object, "author", recipe.m_author);
	CookServerHelper::addString(object, "authorUserId", recipe.m_authorUserId);
	CookServerHelper::addInt(object, "priceCoins", recipe.m_priceCoins);
	CookServerHelper::addString(object, "priceType", recipe.m_priceType);
	CookServerHelper::addInt(object, "priceAmount", recipe.m_priceAmount);
	CookServerHelper::addString(object, "status", recipe.m_status);
	CookServerHelper::addString(object, "currentRevisionId", recipe.m_currentRevisionId);
	CookServerHelper::addString(object, "ownedType", recipe.m_defaultOwned ? "default" : (owned ? "owned" : "none"));
	CookServerHelper::addBool(object, "owned", owned);
	CookServerHelper::addBool(object, "accessible", accessible);
	CookServerHelper::addBool(object, "defaultOwned", recipe.m_defaultOwned);
	CookServerHelper::addBool(object, "favorite", favorite);
	CookServerHelper::addBool(object, "systemRecipe", recipe.m_systemRecipe);
	CookServerHelper::addBool(object, "customRecipe", recipe.m_customRecipe);
	CookServerHelper::addBool(object, "personalizationApplied", recipe.m_personalizationApplied);
	CookServerHelper::addString(object, "personalizationSummary", recipe.m_personalizationSummary);
	CookServerHelper::addStringArray(object, "tags", recipe.m_tags);

	RapidJsonValue ingredients;
	ingredients.setArray();
	ingredients.reserve(recipe.m_ingredients.size());
	for (size_t i = 0; i < recipe.m_ingredients.size(); ++i)
	{
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "name", recipe.m_ingredients[i].m_name);
		CookServerHelper::addString(item, "amount", recipe.m_ingredients[i].m_amount);
		ingredients.pushValue(item);
	}
	object.addValue("ingredients", ingredients);
	CookServerHelper::addStringArray(object, "tools", recipe.m_tools);

	if (accessible)
	{
		RapidJsonValue tasks;
		tasks.setArray();
		tasks.reserve(recipe.m_tasks.size());
		for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
		{
			RapidJsonValue task;
			task.setObject();
			CookServerHelper::addString(task, "id", recipe.m_tasks[i].m_id);
			CookServerHelper::addString(task, "shortId", recipe.m_tasks[i].m_shortId);
			CookServerHelper::addString(task, "recipeId", recipe.m_tasks[i].m_recipeId);
			CookServerHelper::addString(task, "title", recipe.m_tasks[i].m_title);
			CookServerHelper::addString(task, "detail", recipe.m_tasks[i].m_detail);
			CookServerHelper::addString(task, "resource", recipe.m_tasks[i].m_resource);
			CookServerHelper::addString(task, "voiceText", recipe.m_tasks[i].m_voiceText);
			CookServerHelper::addString(task, "safetyLevel", recipe.m_tasks[i].m_safetyLevel);
			CookServerHelper::addString(task, "backgroundWaitMode", recipe.m_tasks[i].m_backgroundWaitMode);
			CookServerHelper::addInt(task, "durationSeconds", recipe.m_tasks[i].m_durationSeconds);
			CookServerHelper::addBool(task, "active", recipe.m_tasks[i].m_active);
			CookServerHelper::addBool(task, "canPause", recipe.m_tasks[i].m_canPause);
			CookServerHelper::addBool(task, "continuesDuringPause", recipe.m_tasks[i].m_continuesDuringPause);
			CookServerHelper::addBool(task, "canLeaveKitchen", recipe.m_tasks[i].m_canLeaveKitchen);
			CookServerHelper::addString(task, "taskMode", recipe.m_tasks[i].m_active ? "foreground" : "background");
			CookServerHelper::addBool(task, "foreground", recipe.m_tasks[i].m_active);
			CookServerHelper::addBool(task, "background", !recipe.m_tasks[i].m_active);
			CookServerHelper::addBool(task, "manualSkippable", recipe.m_tasks[i].m_manualSkippable);
			tasks.pushValue(task);
		}
		object.addValue("tasks", tasks);
	}

	PlanResult single = CookSchedule::buildPlanFromRecipes(std::vector<Recipe>(1, recipe));
	CookServerHelper::addInt(object, "estimatedSeconds", single.m_totalSeconds);
	CookServerHelper::addInt(object, "estimatedMinutes", CookServerHelper::ceilMinutes(single.m_totalSeconds));
	CookServerHelper::addInt(object, "activeSeconds", single.m_activeSeconds);
	CookServerHelper::addInt(object, "activeMinutes", CookServerHelper::ceilMinutes(single.m_activeSeconds));
	return object;
}

RapidJsonValue CookServerHelper::recipeSummaryToJson(const Recipe& recipe, const UserAccount* account)
{
	RapidJsonValue object;
	object.setObject();
	bool owned = account == nullptr ? recipe.m_defaultOwned : AccountStore::isRecipeOwned(recipe, *account);
	bool accessible = owned || recipe.m_priceType == "free" || (recipe.m_priceAmount <= 0 && recipe.m_priceCoins <= 0);
	bool favorite = account != nullptr && account->m_favoriteRecipeIds.find(recipe.m_id) != account->m_favoriteRecipeIds.end();

	CookServerHelper::addString(object, "id", recipe.m_id);
	CookServerHelper::addString(object, "title", recipe.m_title);
	CookServerHelper::addString(object, "category", recipe.m_category);
	CookServerHelper::addString(object, "subtitle", recipe.m_subtitle);
	CookServerHelper::addString(object, "difficulty", recipe.m_difficulty);
	CookServerHelper::addString(object, "coverColor", recipe.m_coverColor);
	CookServerHelper::addString(object, "author", recipe.m_author);
	CookServerHelper::addString(object, "authorUserId", recipe.m_authorUserId);
	CookServerHelper::addInt(object, "priceCoins", recipe.m_priceCoins);
	CookServerHelper::addString(object, "priceType", recipe.m_priceType);
	CookServerHelper::addInt(object, "priceAmount", recipe.m_priceAmount);
	CookServerHelper::addString(object, "status", recipe.m_status);
	CookServerHelper::addString(object, "currentRevisionId", recipe.m_currentRevisionId);
	CookServerHelper::addString(object, "ownedType", recipe.m_defaultOwned ? "default" : (owned ? "owned" : "none"));
	CookServerHelper::addBool(object, "owned", owned);
	CookServerHelper::addBool(object, "accessible", accessible);
	CookServerHelper::addBool(object, "defaultOwned", recipe.m_defaultOwned);
	CookServerHelper::addBool(object, "favorite", favorite);
	CookServerHelper::addBool(object, "systemRecipe", recipe.m_systemRecipe);
	CookServerHelper::addBool(object, "customRecipe", recipe.m_customRecipe);
	CookServerHelper::addBool(object, "personalizationApplied", recipe.m_personalizationApplied);
	CookServerHelper::addString(object, "personalizationSummary", recipe.m_personalizationSummary);
	CookServerHelper::addStringArray(object, "tags", recipe.m_tags);

	PlanResult single = CookSchedule::buildPlanFromRecipes(std::vector<Recipe>(1, recipe));
	CookServerHelper::addInt(object, "estimatedSeconds", single.m_totalSeconds);
	CookServerHelper::addInt(object, "estimatedMinutes", CookServerHelper::ceilMinutes(single.m_totalSeconds));
	CookServerHelper::addInt(object, "activeSeconds", single.m_activeSeconds);
	CookServerHelper::addInt(object, "activeMinutes", CookServerHelper::ceilMinutes(single.m_activeSeconds));
	return object;
}

RapidJsonValue CookServerHelper::planToJson(const PlanResult& plan, const UserAccount& account, bool includePlanId, const std::string& planId, bool lite)
{
	RapidJsonValue root;
	root.setObject();
	root.addBool("ok", true);
	root.addBool("lite", lite);
	const int32_t recipeTotal = static_cast<int32_t>(plan.m_recipes.size());
	const int32_t timelineTotal = static_cast<int32_t>(plan.m_timeline.size());
	if (includePlanId)
	{
		CookServerHelper::addString(root, "planId", planId.empty() ? "plan_" + CStringManager::toStringInt32(static_cast<int32_t>(std::time(nullptr))) : planId);
	}

	RapidJsonValue summary;
	summary.setObject();
	CookServerHelper::addInt(summary, "selectedCount", static_cast<int32_t>(plan.m_recipes.size()));
	CookServerHelper::addInt(summary, "totalSeconds", plan.m_totalSeconds);
	CookServerHelper::addInt(summary, "totalMinutes", CookServerHelper::ceilMinutes(plan.m_totalSeconds));
	CookServerHelper::addInt(summary, "activeSeconds", plan.m_activeSeconds);
	CookServerHelper::addInt(summary, "activeMinutes", CookServerHelper::ceilMinutes(plan.m_activeSeconds));
	CookServerHelper::addInt(summary, "freeSeconds", plan.m_freeSeconds);
	CookServerHelper::addInt(summary, "freeMinutes", CookServerHelper::ceilMinutes(plan.m_freeSeconds));
	CookServerHelper::addInt(summary, "edgeSeconds", plan.m_edgeSeconds);
	CookServerHelper::addInt(summary, "edgeMinutes", CookServerHelper::ceilMinutes(plan.m_edgeSeconds));
	CookServerHelper::addInt(summary, "watchSeconds", plan.m_edgeSeconds);
	CookServerHelper::addInt(summary, "watchMinutes", CookServerHelper::ceilMinutes(plan.m_edgeSeconds));
	CookServerHelper::addInt(summary, "freeBlockMinimumSeconds", g_config.m_freeBlockMinimumSeconds);
	CookServerHelper::addString(summary, "headline", "预计 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_totalSeconds)) + " 分钟吃上，真正动手约 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_activeSeconds)) + " 分钟");
	root.addValue("summary", summary);

	RapidJsonValue selectedRecipes;
	selectedRecipes.setArray();
	selectedRecipes.reserve(plan.m_recipes.size());
	std::map<std::string, std::string> recipeTitles;
	RapidJsonValue recipeIds;
	recipeIds.setArray();
	recipeIds.reserve(plan.m_recipes.size());
	for (size_t i = 0; i < plan.m_recipes.size(); ++i)
	{
		if (!lite)
		{
			selectedRecipes.pushValue(CookServerHelper::recipeToJson(*plan.m_recipes[i], &account));
		}
		recipeTitles[plan.m_recipes[i]->m_id] = plan.m_recipes[i]->m_title;
		CookServerHelper::pushString(recipeIds, plan.m_recipes[i]->m_id);
	}
	root.addValue("recipeIds", recipeIds);
	if (!lite)
	{
		root.addValue("selectedRecipes", selectedRecipes);
		CookServerHelper::addInt(root, "selectedRecipesOffset", 0);
		CookServerHelper::addInt(root, "selectedRecipesCount", recipeTotal);
		CookServerHelper::addInt(root, "selectedRecipesTotal", recipeTotal);
	}

	RapidJsonValue timeline;
	timeline.setArray();
	timeline.reserve(plan.m_timeline.size());
	for (size_t i = 0; i < plan.m_timeline.size(); ++i)
	{
		const ScheduledTask& task = plan.m_timeline[i];
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "id", task.m_id);
		CookServerHelper::addString(item, "recipeId", task.m_recipeId);
		std::map<std::string, std::string>::const_iterator recipeTitleIt = recipeTitles.find(task.m_recipeId);
		CookServerHelper::addString(item, "recipeTitle", recipeTitleIt == recipeTitles.end() ? task.m_recipeId : recipeTitleIt->second);
		CookServerHelper::addString(item, "title", task.m_title);
		CookServerHelper::addString(item, "detail", task.m_detail);
		if (!lite)
		{
			CookServerHelper::addString(item, "resource", task.m_resource);
			CookServerHelper::addString(item, "voiceText", task.m_voiceText);
		}
		CookServerHelper::addString(item, "safetyLevel", task.m_safetyLevel);
		CookServerHelper::addString(item, "backgroundWaitMode", task.m_backgroundWaitMode);
		CookServerHelper::addInt(item, "startSeconds", task.m_startSeconds);
		CookServerHelper::addInt(item, "endSeconds", task.m_endSeconds);
		CookServerHelper::addInt(item, "durationSeconds", task.m_durationSeconds);
		CookServerHelper::addBool(item, "active", task.m_active);
		CookServerHelper::addBool(item, "canPause", task.m_canPause);
		CookServerHelper::addBool(item, "continuesDuringPause", task.m_continuesDuringPause);
		CookServerHelper::addBool(item, "canLeaveKitchen", task.m_canLeaveKitchen);
		CookServerHelper::addString(item, "taskMode", task.m_active ? "foreground" : "background");
		if (!lite)
		{
			CookServerHelper::addBool(item, "foreground", task.m_active);
			CookServerHelper::addBool(item, "background", !task.m_active);
		}
		CookServerHelper::addBool(item, "manualSkippable", task.m_manualSkippable);
		RapidJsonValue dependencies;
		dependencies.setArray();
		dependencies.reserve(task.m_dependencies.size());
		for (size_t j = 0; j < task.m_dependencies.size(); ++j)
		{
			CookServerHelper::pushString(dependencies, task.m_dependencies[j]);
		}
		item.addValue("dependencies", dependencies);
		timeline.pushValue(item);
	}
	root.addValue("timeline", timeline);
	CookServerHelper::addInt(root, "timelineOffset", 0);
	CookServerHelper::addInt(root, "timelineCount", timelineTotal);
	CookServerHelper::addInt(root, "timelineTotal", timelineTotal);

	RapidJsonValue freeBlocks;
	freeBlocks.setArray();
	freeBlocks.reserve(plan.m_freeBlocks.size());
	for (size_t i = 0; i < plan.m_freeBlocks.size(); ++i)
	{
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addInt(item, "startSeconds", plan.m_freeBlocks[i].m_startSeconds);
		CookServerHelper::addInt(item, "endSeconds", plan.m_freeBlocks[i].m_endSeconds);
		CookServerHelper::addInt(item, "durationSeconds", plan.m_freeBlocks[i].m_endSeconds - plan.m_freeBlocks[i].m_startSeconds);
		freeBlocks.pushValue(item);
	}
	root.addValue("freeBlocks", freeBlocks);

	RapidJsonValue warnings;
	warnings.setArray();
	warnings.reserve(plan.m_warnings.size());
	for (size_t i = 0; i < plan.m_warnings.size(); ++i)
	{
		CookServerHelper::pushString(warnings, plan.m_warnings[i]);
	}
	root.addValue("warnings", warnings);
	return root;
}

RapidJsonValue CookServerHelper::planSummaryToJson(const PlanResult& plan, bool includePlanId, bool includeTimeBlocks)
{
	RapidJsonValue root;
	root.setObject();
	root.addBool("ok", true);
	CookServerHelper::addBool(root, "summaryOnly", true);
	CookServerHelper::addBool(root, "includeTimeBlocks", includeTimeBlocks);
	if (includePlanId)
	{
		CookServerHelper::addString(root, "planId", "plan_" + CStringManager::toStringInt32(static_cast<int32_t>(std::time(nullptr))));
	}

	RapidJsonValue summary;
	summary.setObject();
	CookServerHelper::addInt(summary, "selectedCount", static_cast<int32_t>(plan.m_recipes.size()));
	CookServerHelper::addInt(summary, "totalSeconds", plan.m_totalSeconds);
	CookServerHelper::addInt(summary, "totalMinutes", CookServerHelper::ceilMinutes(plan.m_totalSeconds));
	CookServerHelper::addInt(summary, "activeSeconds", plan.m_activeSeconds);
	CookServerHelper::addInt(summary, "activeMinutes", CookServerHelper::ceilMinutes(plan.m_activeSeconds));
	CookServerHelper::addInt(summary, "freeSeconds", plan.m_freeSeconds);
	CookServerHelper::addInt(summary, "freeMinutes", CookServerHelper::ceilMinutes(plan.m_freeSeconds));
	CookServerHelper::addInt(summary, "edgeSeconds", plan.m_edgeSeconds);
	CookServerHelper::addInt(summary, "edgeMinutes", CookServerHelper::ceilMinutes(plan.m_edgeSeconds));
	CookServerHelper::addInt(summary, "watchSeconds", plan.m_edgeSeconds);
	CookServerHelper::addInt(summary, "watchMinutes", CookServerHelper::ceilMinutes(plan.m_edgeSeconds));
	CookServerHelper::addInt(summary, "freeBlockMinimumSeconds", g_config.m_freeBlockMinimumSeconds);
	CookServerHelper::addString(summary, "headline", "预计 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_totalSeconds)) + " 分钟吃上，真正动手约 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_activeSeconds)) + " 分钟");
	root.addValue("summary", summary);

	RapidJsonValue recipeIds;
	recipeIds.setArray();
	recipeIds.reserve(plan.m_recipes.size());
	for (size_t i = 0; i < plan.m_recipes.size(); ++i)
	{
		CookServerHelper::pushString(recipeIds, plan.m_recipes[i]->m_id);
	}
	root.addValue("recipeIds", recipeIds);

	if (includeTimeBlocks)
	{
		RapidJsonValue freeBlocks;
		freeBlocks.setArray();
		freeBlocks.reserve(plan.m_freeBlocks.size());
		for (size_t i = 0; i < plan.m_freeBlocks.size(); ++i)
		{
			RapidJsonValue item;
			item.setObject();
			CookServerHelper::addString(item, "type", "free");
			CookServerHelper::addInt(item, "startSeconds", plan.m_freeBlocks[i].m_startSeconds);
			CookServerHelper::addInt(item, "endSeconds", plan.m_freeBlocks[i].m_endSeconds);
			CookServerHelper::addInt(item, "durationSeconds", plan.m_freeBlocks[i].m_endSeconds - plan.m_freeBlocks[i].m_startSeconds);
			freeBlocks.pushValue(item);
		}
		root.addValue("freeBlocks", freeBlocks);

		struct Segment
		{
			std::string m_type;
			int32_t m_startSeconds;
			int32_t m_endSeconds;
		};

		std::vector<std::pair<int32_t, int32_t>> activeBlocks;
		std::vector<std::pair<int32_t, int32_t>> freeRanges;
		std::vector<int32_t> points;
		points.push_back(0);
		points.push_back(plan.m_totalSeconds);
		for (size_t i = 0; i < plan.m_timeline.size(); ++i)
		{
			if (!plan.m_timeline[i].m_active)
			{
				continue;
			}
			activeBlocks.push_back(std::make_pair(plan.m_timeline[i].m_startSeconds, plan.m_timeline[i].m_endSeconds));
			points.push_back(plan.m_timeline[i].m_startSeconds);
			points.push_back(plan.m_timeline[i].m_endSeconds);
		}
		for (size_t i = 0; i < plan.m_freeBlocks.size(); ++i)
		{
			freeRanges.push_back(std::make_pair(plan.m_freeBlocks[i].m_startSeconds, plan.m_freeBlocks[i].m_endSeconds));
			points.push_back(plan.m_freeBlocks[i].m_startSeconds);
			points.push_back(plan.m_freeBlocks[i].m_endSeconds);
		}
		std::sort(points.begin(), points.end());
		points.erase(std::unique(points.begin(), points.end()), points.end());

		std::vector<Segment> segments;
		for (size_t i = 1; i < points.size(); ++i)
		{
			int32_t startSeconds = points[i - 1];
			int32_t endSeconds = points[i];
			if (endSeconds <= startSeconds)
			{
				continue;
			}

			std::string type = "watch";
			for (size_t j = 0; j < activeBlocks.size(); ++j)
			{
				if (activeBlocks[j].first < endSeconds && activeBlocks[j].second > startSeconds)
				{
					type = "active";
					break;
				}
			}
			if (type == "watch")
			{
				for (size_t j = 0; j < freeRanges.size(); ++j)
				{
					if (freeRanges[j].first < endSeconds && freeRanges[j].second > startSeconds)
					{
						type = "free";
						break;
					}
				}
			}

			if (!segments.empty() && segments.back().m_type == type && segments.back().m_endSeconds == startSeconds)
			{
				segments.back().m_endSeconds = endSeconds;
			}
			else
			{
				Segment segment;
				segment.m_type = type;
				segment.m_startSeconds = startSeconds;
				segment.m_endSeconds = endSeconds;
				segments.push_back(segment);
			}
		}

		RapidJsonValue timeSegments;
		timeSegments.setArray();
		timeSegments.reserve(segments.size());
		RapidJsonValue watchBlocks;
		watchBlocks.setArray();
		for (size_t i = 0; i < segments.size(); ++i)
		{
			RapidJsonValue item;
			item.setObject();
			CookServerHelper::addString(item, "type", segments[i].m_type);
			CookServerHelper::addInt(item, "startSeconds", segments[i].m_startSeconds);
			CookServerHelper::addInt(item, "endSeconds", segments[i].m_endSeconds);
			CookServerHelper::addInt(item, "durationSeconds", segments[i].m_endSeconds - segments[i].m_startSeconds);
			timeSegments.pushValue(item);
			if (segments[i].m_type == "watch")
			{
				watchBlocks.pushValue(item);
			}
		}
		root.addValue("watchBlocks", watchBlocks);
		root.addValue("timeSegments", timeSegments);
	}

	RapidJsonValue warnings;
	warnings.setArray();
	warnings.reserve(plan.m_warnings.size());
	for (size_t i = 0; i < plan.m_warnings.size(); ++i)
	{
		CookServerHelper::pushString(warnings, plan.m_warnings[i]);
	}
	root.addValue("warnings", warnings);
	return root;
}

HttpResponse CookServerHelper::jsonResponse(const std::string& body, int32_t statusCode)
{
	return HttpResponse::json(body, statusCode);
}

const char* CookServerHelper::routeAt(size_t index)
{
	return index < CookServerHelper::routeCount() ? kCookServerRoutes[index] : "";
}

size_t CookServerHelper::routeCount()
{
	return sizeof(kCookServerRoutes) / sizeof(kCookServerRoutes[0]);
}