#include "CookServerHelper.h"
#include "AccountStore.h"
#include "CookCatalog.h"
#include "CookSchedule.h"
#include "CStringManager/CStringManagerAPI.h"
#include <ctime>

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
	CookServerHelper::addInt(object, "coins", account.m_coins);

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
	return object;
}

RapidJsonValue CookServerHelper::recipeToJson(const Recipe& recipe, const UserAccount* account)
{
	RapidJsonValue object;
	object.setObject();
	bool owned = account == nullptr ? recipe.m_defaultOwned : AccountStore::isRecipeOwned(recipe, *account);
	bool favorite = account != nullptr && account->m_favoriteRecipeIds.find(recipe.m_id) != account->m_favoriteRecipeIds.end();

	CookServerHelper::addString(object, "id", recipe.m_id);
	CookServerHelper::addString(object, "title", recipe.m_title);
	CookServerHelper::addString(object, "category", recipe.m_category);
	CookServerHelper::addString(object, "subtitle", recipe.m_subtitle);
	CookServerHelper::addString(object, "difficulty", recipe.m_difficulty);
	CookServerHelper::addString(object, "coverColor", recipe.m_coverColor);
	CookServerHelper::addString(object, "author", recipe.m_author);
	CookServerHelper::addInt(object, "priceCoins", recipe.m_priceCoins);
	CookServerHelper::addBool(object, "owned", owned);
	CookServerHelper::addBool(object, "defaultOwned", recipe.m_defaultOwned);
	CookServerHelper::addBool(object, "favorite", favorite);
	CookServerHelper::addBool(object, "systemRecipe", recipe.m_systemRecipe);
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

	PlanResult single = CookSchedule::buildPlan(std::vector<std::string>(1, recipe.m_id));
	CookServerHelper::addInt(object, "estimatedSeconds", single.m_totalSeconds);
	CookServerHelper::addInt(object, "estimatedMinutes", CookServerHelper::ceilMinutes(single.m_totalSeconds));
	CookServerHelper::addInt(object, "activeSeconds", single.m_activeSeconds);
	CookServerHelper::addInt(object, "activeMinutes", CookServerHelper::ceilMinutes(single.m_activeSeconds));
	return object;
}

RapidJsonValue CookServerHelper::planToJson(const PlanResult& plan, const UserAccount& account, bool includePlanId)
{
	RapidJsonValue root;
	root.setObject();
	root.addBool("ok", true);
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
	CookServerHelper::addString(summary, "headline", "预计 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_totalSeconds)) + " 分钟吃上，真正动手约 " + CStringManager::toStringInt32(CookServerHelper::ceilMinutes(plan.m_activeSeconds)) + " 分钟");
	root.addValue("summary", summary);

	RapidJsonValue selectedRecipes;
	selectedRecipes.setArray();
	selectedRecipes.reserve(plan.m_recipes.size());
	for (size_t i = 0; i < plan.m_recipes.size(); ++i)
	{
		selectedRecipes.pushValue(CookServerHelper::recipeToJson(*plan.m_recipes[i], &account));
	}
	root.addValue("selectedRecipes", selectedRecipes);

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
		const Recipe* recipe = CookCatalog::findRecipe(task.m_recipeId);
		CookServerHelper::addString(item, "recipeTitle", recipe == nullptr ? task.m_recipeId : recipe->m_title);
		CookServerHelper::addString(item, "title", task.m_title);
		CookServerHelper::addString(item, "detail", task.m_detail);
		CookServerHelper::addString(item, "resource", task.m_resource);
		CookServerHelper::addString(item, "voiceText", task.m_voiceText);
		CookServerHelper::addString(item, "safetyLevel", task.m_safetyLevel);
		CookServerHelper::addInt(item, "startSeconds", task.m_startSeconds);
		CookServerHelper::addInt(item, "endSeconds", task.m_endSeconds);
		CookServerHelper::addInt(item, "durationSeconds", task.m_durationSeconds);
		CookServerHelper::addBool(item, "active", task.m_active);
		CookServerHelper::addBool(item, "canPause", task.m_canPause);
		CookServerHelper::addBool(item, "continuesDuringPause", task.m_continuesDuringPause);
		CookServerHelper::addBool(item, "canLeaveKitchen", task.m_canLeaveKitchen);
		timeline.pushValue(item);
	}
	root.addValue("timeline", timeline);

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
	return root;
}

HttpResponse CookServerHelper::jsonResponse(const std::string& body, int32_t statusCode)
{
	return HttpResponse::json(body, statusCode);
}

const char* CookServerHelper::routeAt(size_t index)
{
	switch (index)
	{
	case 0:
	{
		return "/api/health";
	}
	case 1:
	{
		return "/api/recipes";
	}
	case 2:
	{
		return "/api/feed";
	}
	case 3:
	{
		return "/api/account";
	}
	case 4:
	{
		return "/api/account/recharge";
	}
	case 5:
	{
		return "/api/recipes/purchase";
	}
	case 6:
	{
		return "/api/favorites/toggle";
	}
	case 7:
	{
		return "/api/plan/preview";
	}
	case 8:
	{
		return "/api/plan/start";
	}
	default:
	{
		return "";
	}
	}
}

size_t CookServerHelper::routeCount()
{
	return 9;
}