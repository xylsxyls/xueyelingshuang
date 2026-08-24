#include "CookRecipeConfig.h"
#include "CookServerHelper.h"
#include "CSystem/CSystemAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <set>

bool CookRecipeConfig::loadRecipeFromFile(const std::string& filePath, Recipe& recipe, std::string* errorMessage)
{
	if (filePath.empty())
	{
		return CookRecipeConfig::setError(errorMessage, "recipe file path is empty");
	}
	if (!CSystem::DirOrFileExist(filePath))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe file not found: " + filePath);
	}
	std::string json = CSystem::readFile(filePath);
	if (json.empty())
	{
		return CookRecipeConfig::setError(errorMessage, "recipe file is empty: " + filePath);
	}
	return CookRecipeConfig::loadRecipeFromJson(json, recipe, errorMessage);
}

bool CookRecipeConfig::loadRecipeFromJson(const std::string& json, Recipe& recipe, std::string* errorMessage)
{
	std::string realJson = CookRecipeConfig::removeUtf8Bom(json);
	if (!CookServerHelper::isUtf8Text(realJson))
	{
		realJson = CookServerHelper::textToJsonUtf8(realJson);
	}
	if (!CookServerHelper::isUtf8Text(realJson))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe json must be UTF-8");
	}
	RapidJsonDocument document;
	if (!document.parse(realJson) || !document.isObject())
	{
		return CookRecipeConfig::setError(errorMessage, "recipe json parse failed");
	}

	std::string schemaVersion = document.getStringOrDefault("schemaVersion", "");
	if (schemaVersion == "1.0")
	{
		return CookRecipeConfig::loadVersion10(document, recipe, errorMessage);
	}
	return CookRecipeConfig::setError(errorMessage, "recipe schemaVersion is unsupported: " + schemaVersion);
}

std::string CookRecipeConfig::supportedSchemaVersion()
{
	return "1.0";
}

bool CookRecipeConfig::isMissingString(const std::string& text)
{
	return text.empty();
}

std::string CookRecipeConfig::removeUtf8Bom(const std::string& json)
{
	if (json.size() >= 3 &&
		static_cast<unsigned char>(json[0]) == 0xEF &&
		static_cast<unsigned char>(json[1]) == 0xBB &&
		static_cast<unsigned char>(json[2]) == 0xBF)
	{
		return json.substr(3);
	}
	return json;
}

Ingredient CookRecipeConfig::parseIngredient(const RapidJsonValue& value)
{
	Ingredient ingredient;
	ingredient.m_name = value.getStringOrDefault("name", "");
	ingredient.m_amount = value.getStringOrDefault("amount", "");
	return ingredient;
}

bool CookRecipeConfig::parseTask(const RapidJsonValue& value, const std::string& recipeId, CookTask& task, std::string* errorMessage)
{
	std::string shortId = value.getStringOrDefault("id", "");
	if (CookRecipeConfig::isMissingString(shortId))
	{
		return CookRecipeConfig::setError(errorMessage, "task id is empty in recipe: " + recipeId);
	}
	task.m_id = recipeId + "." + shortId;
	task.m_shortId = shortId;
	task.m_recipeId = recipeId;
	task.m_title = value.getStringOrDefault("title", "");
	task.m_detail = value.getStringOrDefault("detail", "");
	task.m_resource = value.getStringOrDefault("resource", "");
	task.m_voiceText = value.getStringOrDefault("voiceText", "");
	task.m_safetyLevel = value.getStringOrDefault("safetyLevel", "normal");
	task.m_backgroundWaitMode = value.getStringOrDefault("backgroundWaitMode", "");
	task.m_durationSeconds = value.getIntOrDefault("durationSeconds", -1);
	std::string taskMode = value.getStringOrDefault("taskMode", "");
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
		task.m_active = value.getBoolOrDefault("active", false);
	}
	task.m_manualSkippable = value.getBoolOrDefault("manualSkippable", task.m_active);
	task.m_canPause = value.getBoolOrDefault("canPause", false);
	task.m_continuesDuringPause = value.getBoolOrDefault("continuesDuringPause", false);
	task.m_canLeaveKitchen = value.getBoolOrDefault("canLeaveKitchen", false);
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
	if (CookRecipeConfig::isMissingString(task.m_title))
	{
		return CookRecipeConfig::setError(errorMessage, "task title is empty: " + task.m_id);
	}
	if (task.m_durationSeconds < 0)
	{
		return CookRecipeConfig::setError(errorMessage, "task durationSeconds is invalid: " + task.m_id);
	}
	if (task.m_voiceText.empty())
	{
		task.m_voiceText = CookServerHelper::joinJsonUtf8Text(task.m_title, "。", task.m_detail);
	}
	std::vector<std::string> dependencies = value.getStringArrayOrEmpty("dependencies");
	for (size_t i = 0; i < dependencies.size(); ++i)
	{
		if (!dependencies[i].empty())
		{
			std::string prefix = recipeId + ".";
			task.m_dependencies.push_back(dependencies[i].find(prefix) == 0 ? dependencies[i] : prefix + dependencies[i]);
		}
	}
	return true;
}

bool CookRecipeConfig::validateRecipeTasks(const Recipe& recipe, std::string* errorMessage)
{
	std::set<std::string> taskIds;
	for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
	{
		if (taskIds.find(recipe.m_tasks[i].m_id) != taskIds.end())
		{
			return CookRecipeConfig::setError(errorMessage, "task id duplicated: " + recipe.m_tasks[i].m_id);
		}
		taskIds.insert(recipe.m_tasks[i].m_id);
	}
	for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
	{
		for (size_t j = 0; j < recipe.m_tasks[i].m_dependencies.size(); ++j)
		{
			if (taskIds.find(recipe.m_tasks[i].m_dependencies[j]) == taskIds.end())
			{
				return CookRecipeConfig::setError(errorMessage, "task dependency missing: " + recipe.m_tasks[i].m_dependencies[j]);
			}
		}
	}
	return true;
}

bool CookRecipeConfig::parseRecipe(const RapidJsonValue& value, Recipe& recipe, std::string* errorMessage)
{
	recipe = Recipe();
	recipe.m_id = value.getStringOrDefault("id", "");
	recipe.m_title = value.getStringOrDefault("title", "");
	recipe.m_category = value.getStringOrDefault("category", "");
	recipe.m_subtitle = value.getStringOrDefault("subtitle", "");
	recipe.m_difficulty = value.getStringOrDefault("difficulty", "");
	recipe.m_coverColor = value.getStringOrDefault("coverColor", "");
	recipe.m_author = value.getStringOrDefault("author", "");
	recipe.m_authorUserId = value.getStringOrDefault("authorUserId", value.getStringOrDefault("authorId", ""));
	recipe.m_tags = value.getStringArrayOrEmpty("tags");
	recipe.m_tools = value.getStringArrayOrEmpty("tools");
	recipe.m_priceCoins = value.getIntOrDefault("priceCoins", 0);
	recipe.m_priceType = value.getStringOrDefault("priceType", "");
	recipe.m_priceAmount = value.getIntOrDefault("priceAmount", recipe.m_priceCoins);
	recipe.m_status = value.getStringOrDefault("status", "published");
	recipe.m_currentRevisionId = value.getStringOrDefault("currentRevisionId", recipe.m_id + "_r1");
	recipe.m_defaultOwned = value.getBoolOrDefault("defaultOwned", value.getBoolOrDefault("owned", recipe.m_priceCoins == 0));
	recipe.m_systemRecipe = value.getBoolOrDefault("systemRecipe", false);
	recipe.m_customRecipe = value.getBoolOrDefault("customRecipe", !recipe.m_systemRecipe);
	if (recipe.m_priceType.empty() && recipe.m_id == "cook_000007" && recipe.m_priceAmount > 0)
	{
		recipe.m_priceType = "yuanbao";
	}
	if (recipe.m_priceType.empty())
	{
		recipe.m_priceType = recipe.m_priceAmount > 0 ? "coin" : "free";
	}
	if (recipe.m_priceAmount <= 0 && recipe.m_priceCoins > 0)
	{
		recipe.m_priceAmount = recipe.m_priceCoins;
	}
	if (recipe.m_priceType == "free")
	{
		recipe.m_priceAmount = 0;
		recipe.m_priceCoins = 0;
	}
	recipe.m_defaultOwned = value.getBoolOrDefault("defaultOwned", value.getBoolOrDefault("owned", recipe.m_priceType == "free" || recipe.m_priceAmount == 0));
	if (CookRecipeConfig::isMissingString(recipe.m_id))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe id is empty");
	}
	if (CookRecipeConfig::isMissingString(recipe.m_title))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe title is empty: " + recipe.m_id);
	}
	if (recipe.m_priceCoins < 0)
	{
		return CookRecipeConfig::setError(errorMessage, "recipe priceCoins is invalid: " + recipe.m_id);
	}
	if (recipe.m_priceAmount < 0)
	{
		return CookRecipeConfig::setError(errorMessage, "recipe priceAmount is invalid: " + recipe.m_id);
	}

	std::vector<RapidJsonValue> ingredientValues = value.getArrayValueOrEmpty("ingredients");
	recipe.m_ingredients.reserve(ingredientValues.size());
	for (size_t i = 0; i < ingredientValues.size(); ++i)
	{
		Ingredient ingredient = CookRecipeConfig::parseIngredient(ingredientValues[i]);
		if (!ingredient.m_name.empty())
		{
			recipe.m_ingredients.push_back(ingredient);
		}
	}

	std::vector<RapidJsonValue> taskValues = value.getArrayValueOrEmpty("tasks");
	if (taskValues.empty())
	{
		return CookRecipeConfig::setError(errorMessage, "recipe tasks is empty: " + recipe.m_id);
	}
	recipe.m_tasks.reserve(taskValues.size());
	for (size_t i = 0; i < taskValues.size(); ++i)
	{
		CookTask task;
		if (!CookRecipeConfig::parseTask(taskValues[i], recipe.m_id, task, errorMessage))
		{
			return false;
		}
		recipe.m_tasks.push_back(task);
	}
	return CookRecipeConfig::validateRecipeTasks(recipe, errorMessage);
}

bool CookRecipeConfig::parseRecipe(const RapidJsonDocument& document, Recipe& recipe, std::string* errorMessage)
{
	recipe = Recipe();
	recipe.m_id = document.getStringOrDefault("id", "");
	recipe.m_title = document.getStringOrDefault("title", "");
	recipe.m_category = document.getStringOrDefault("category", "");
	recipe.m_subtitle = document.getStringOrDefault("subtitle", "");
	recipe.m_difficulty = document.getStringOrDefault("difficulty", "");
	recipe.m_coverColor = document.getStringOrDefault("coverColor", "");
	recipe.m_author = document.getStringOrDefault("author", "");
	recipe.m_authorUserId = document.getStringOrDefault("authorUserId", document.getStringOrDefault("authorId", ""));
	recipe.m_tags = document.getStringArrayOrEmpty("tags");
	recipe.m_tools = document.getStringArrayOrEmpty("tools");
	recipe.m_priceCoins = document.getIntOrDefault("priceCoins", 0);
	recipe.m_priceType = document.getStringOrDefault("priceType", "");
	recipe.m_priceAmount = document.getIntOrDefault("priceAmount", recipe.m_priceCoins);
	recipe.m_status = document.getStringOrDefault("status", "published");
	recipe.m_currentRevisionId = document.getStringOrDefault("currentRevisionId", recipe.m_id + "_r1");
	recipe.m_defaultOwned = document.getBoolOrDefault("defaultOwned", document.getBoolOrDefault("owned", recipe.m_priceCoins == 0));
	recipe.m_systemRecipe = document.getBoolOrDefault("systemRecipe", false);
	recipe.m_customRecipe = document.getBoolOrDefault("customRecipe", !recipe.m_systemRecipe);
	if (recipe.m_priceType.empty() && recipe.m_id == "cook_000007" && recipe.m_priceAmount > 0)
	{
		recipe.m_priceType = "yuanbao";
	}
	if (recipe.m_priceType.empty())
	{
		recipe.m_priceType = recipe.m_priceAmount > 0 ? "coin" : "free";
	}
	if (recipe.m_priceAmount <= 0 && recipe.m_priceCoins > 0)
	{
		recipe.m_priceAmount = recipe.m_priceCoins;
	}
	if (recipe.m_priceType == "free")
	{
		recipe.m_priceAmount = 0;
		recipe.m_priceCoins = 0;
	}
	recipe.m_defaultOwned = document.getBoolOrDefault("defaultOwned", document.getBoolOrDefault("owned", recipe.m_priceType == "free" || recipe.m_priceAmount == 0));
	if (CookRecipeConfig::isMissingString(recipe.m_id))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe id is empty");
	}
	if (CookRecipeConfig::isMissingString(recipe.m_title))
	{
		return CookRecipeConfig::setError(errorMessage, "recipe title is empty: " + recipe.m_id);
	}
	if (recipe.m_priceCoins < 0)
	{
		return CookRecipeConfig::setError(errorMessage, "recipe priceCoins is invalid: " + recipe.m_id);
	}
	if (recipe.m_priceAmount < 0)
	{
		return CookRecipeConfig::setError(errorMessage, "recipe priceAmount is invalid: " + recipe.m_id);
	}

	std::vector<RapidJsonValue> ingredientValues = document.getArrayValueOrEmpty("ingredients");
	recipe.m_ingredients.reserve(ingredientValues.size());
	for (size_t i = 0; i < ingredientValues.size(); ++i)
	{
		Ingredient ingredient = CookRecipeConfig::parseIngredient(ingredientValues[i]);
		if (!ingredient.m_name.empty())
		{
			recipe.m_ingredients.push_back(ingredient);
		}
	}

	std::vector<RapidJsonValue> taskValues = document.getArrayValueOrEmpty("tasks");
	if (taskValues.empty())
	{
		return CookRecipeConfig::setError(errorMessage, "recipe tasks is empty: " + recipe.m_id);
	}
	recipe.m_tasks.reserve(taskValues.size());
	for (size_t i = 0; i < taskValues.size(); ++i)
	{
		CookTask task;
		if (!CookRecipeConfig::parseTask(taskValues[i], recipe.m_id, task, errorMessage))
		{
			return false;
		}
		recipe.m_tasks.push_back(task);
	}
	return CookRecipeConfig::validateRecipeTasks(recipe, errorMessage);
}

bool CookRecipeConfig::loadVersion10(const RapidJsonDocument& document, Recipe& recipe, std::string* errorMessage)
{
	RapidJsonValue recipeValue = document.getValue("recipe");
	if (recipeValue.isObject() && !recipeValue.getStringOrDefault("id", "").empty())
	{
		return CookRecipeConfig::parseRecipe(recipeValue, recipe, errorMessage);
	}
	return CookRecipeConfig::parseRecipe(document, recipe, errorMessage);
}

bool CookRecipeConfig::setError(std::string* errorMessage, const std::string& message)
{
	if (errorMessage != nullptr)
	{
		*errorMessage = message;
	}
	return false;
}