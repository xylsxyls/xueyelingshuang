#include "AccountStore.h"
#include "Config.h"
#include "CookCatalog.h"
#include "CookHelper.h"
#include "CookRecipeConfig.h"
#include "CookServerHelper.h"
#include "CookStorageService.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

bool AccountStore::isRecipeOwned(const Recipe& recipe, const UserAccount& account)
{
	return recipe.m_defaultOwned ||
		recipe.m_authorUserId == account.m_userId ||
		account.m_ownedRecipeIds.find(recipe.m_id) != account.m_ownedRecipeIds.end();
}

AccountStore::AccountStore() :
m_sequence(0),
m_resourceVideosSynced(false),
m_lastResourceVideoSyncSeconds(0),
m_loaded(false)
{

}

std::string AccountStore::normalizeUserId(const std::string& userId) const
{
	return userId.empty() ? "demo_user" : userId;
}

std::string AccountStore::normalizeAccountNameNoLock(const std::string& accountName) const
{
	size_t begin = 0;
	while (begin < accountName.size() &&
		(accountName[begin] == ' ' || accountName[begin] == '\t' || accountName[begin] == '\r' || accountName[begin] == '\n'))
	{
		++begin;
	}
	size_t end = accountName.size();
	while (end > begin &&
		(accountName[end - 1] == ' ' || accountName[end - 1] == '\t' || accountName[end - 1] == '\r' || accountName[end - 1] == '\n'))
	{
		--end;
	}
	return accountName.substr(begin, end - begin);
}

std::vector<std::string> AccountStore::splitKeepEmptyNoLock(const std::string& text, char delimiter) const
{
	std::vector<std::string> result;
	size_t begin = 0;
	size_t position = text.find(delimiter, begin);
	while (position != std::string::npos)
	{
		result.push_back(text.substr(begin, position - begin));
		begin = position + 1;
		position = text.find(delimiter, begin);
	}
	result.push_back(text.substr(begin));
	return result;
}

std::string AccountStore::normalizeRecipeIdNoLock(const std::string& recipeId) const
{
	if (recipeId == "sweet_sour_ribs")
	{
		return "cook_000001";
	}
	if (recipeId == "rice")
	{
		return "cook_000002";
	}
	if (recipeId == "tomato_egg")
	{
		return "cook_000003";
	}
	if (recipeId == "garlic_greens")
	{
		return "cook_000004";
	}
	if (recipeId == "miso_tofu_soup")
	{
		return "cook_000005";
	}
	if (recipeId == "black_pepper_beef")
	{
		return "cook_000006";
	}
	if (recipeId == "teriyaki_chicken")
	{
		return "cook_000007";
	}
	return recipeId;
}

void AccountStore::insertRecipeIdsNoLock(std::set<std::string>& recipeIds, const std::vector<std::string>& sourceRecipeIds) const
{
	for (size_t i = 0; i < sourceRecipeIds.size(); ++i)
	{
		std::string recipeId = normalizeRecipeIdNoLock(sourceRecipeIds[i]);
		if (!recipeId.empty())
		{
			recipeIds.insert(recipeId);
		}
	}
}

void AccountStore::refreshOwnedRecipesNoLock(UserAccount& account) const
{
	const std::vector<Recipe>& recipes = CookCatalog::recipeCatalog();
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		if (recipes[i].m_defaultOwned || recipes[i].m_authorUserId == account.m_userId)
		{
			account.m_ownedRecipeIds.insert(recipes[i].m_id);
		}
	}
	account.m_ownedRecipeIds.insert(account.m_purchasedRecipeIds.begin(), account.m_purchasedRecipeIds.end());
	account.m_ownedRecipeIds.insert(account.m_customRecipeIds.begin(), account.m_customRecipeIds.end());
	if (account.m_account.empty())
	{
		account.m_account = account.m_userId;
	}
	if (account.m_nickname.empty())
	{
		account.m_nickname = account.m_account.empty() ? account.m_userId : account.m_account;
	}
	if (account.m_yuanbao < 0)
	{
		account.m_yuanbao = 0;
	}
}

int32_t AccountStore::nowSecondsNoLock() const
{
	return static_cast<int32_t>(std::time(nullptr));
}

std::string AccountStore::todayTextNoLock() const
{
	std::time_t now = std::time(nullptr);
	std::tm timeInfo;
#ifdef _WIN32
	localtime_s(&timeInfo, &now);
#else
	localtime_r(&now, &timeInfo);
#endif
	char buffer[16] = {};
#ifdef _WIN32
	sprintf_s(buffer, sizeof(buffer), "%04d%02d%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
#else
	std::snprintf(buffer, sizeof(buffer), "%04d%02d%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday);
#endif
	return buffer;
}

std::string AccountStore::makeIdNoLock(const std::string& prefix)
{
	++m_sequence;
	std::string seed = prefix + "|" + CStringManager::toStringInt32(nowSecondsNoLock()) + "|" + CStringManager::toStringInt32(m_sequence);
	return prefix + "_" + hashTextNoLock(seed).substr(0, 16);
}

std::string AccountStore::hashTextNoLock(const std::string& text) const
{
	uint64_t hash = 1469598103934665603ULL;
	for (size_t i = 0; i < text.size(); ++i)
	{
		hash ^= static_cast<unsigned char>(text[i]);
		hash *= 1099511628211ULL;
	}
	std::ostringstream oss;
	oss << std::hex << hash;
	return oss.str();
}

std::string AccountStore::safeFieldNoLock(const std::string& text) const
{
	std::string result = text;
	for (size_t i = 0; i < result.size(); ++i)
	{
		if (result[i] == '\t' || result[i] == '\r' || result[i] == '\n')
		{
			result[i] = ' ';
		}
	}
	return result;
}

std::string AccountStore::targetKeyNoLock(const std::string& targetType, const std::string& targetId) const
{
	std::string realType = targetType.empty() ? "recipe" : targetType;
	std::string realTargetId = realType == "video" ? CookHelper::normalizeVideoId(targetId) : targetId;
	return realType + ":" + realTargetId;
}

const Recipe* AccountStore::findBaseRecipeNoLock(const std::string& recipeId) const
{
	std::string realRecipeId = normalizeRecipeIdNoLock(recipeId);
	std::map<std::string, Recipe>::const_iterator customIt = m_customRecipes.find(realRecipeId);
	if (customIt != m_customRecipes.end())
	{
		return &customIt->second;
	}
	return CookCatalog::findRecipe(realRecipeId);
}

std::string AccountStore::resolveVideoFilePathNoLock(const std::string& path) const
{
	std::string normalized = CookHelper::normalizePathSeparators(path);
	if (normalized.empty())
	{
		return "";
	}
#ifdef _WIN32
	if (normalized.size() >= 2 && normalized[1] == ':')
	{
		return normalized;
	}
	if (normalized[0] == '\\' || normalized[0] == '/')
	{
		return normalized;
	}
#else
	if (normalized[0] == '/')
	{
		return normalized;
	}
#endif
	return CookHelper::appendPathSeparator(CSystem::GetCurrentExePath()) + normalized;
}

void AccountStore::syncResourceVideosNoLock(bool force)
{
	int32_t nowSeconds = nowSecondsNoLock();
	if (!force && m_resourceVideosSynced && g_config.m_videoResourceScanIntervalSeconds > 0)
	{
		int32_t elapsedSeconds = nowSeconds - m_lastResourceVideoSyncSeconds;
		if (elapsedSeconds >= 0 && elapsedSeconds < g_config.m_videoResourceScanIntervalSeconds)
		{
			return;
		}
	}
	m_resourceVideosSynced = true;
	m_lastResourceVideoSyncSeconds = nowSeconds;

	std::string resourceDirectory = resolveVideoFilePathNoLock(g_config.m_videoResourceDirectoryName);
	std::set<std::string> foundIds;
	std::vector<std::string> files = CSystem::findFilePath(resourceDirectory, 2, CookHelper::supportedVideoSuffixList());
	std::sort(files.begin(), files.end());
	for (size_t i = 0; i < files.size(); ++i)
	{
		if (!CookHelper::isSupportedVideoFile(files[i]))
		{
			continue;
		}
		std::string videoId = CSystem::GetName(files[i], 3);
		if (videoId.empty())
		{
			continue;
		}
		if (foundIds.find(videoId) != foundIds.end())
		{
			LOGWARNING("AccountStore resource video duplicate id ignored videoId=%s filePath=%s", videoId.c_str(), files[i].c_str());
			continue;
		}
		foundIds.insert(videoId);

		std::map<std::string, VideoInfo>::iterator videoIt = m_videos.find(videoId);
		if (videoIt != m_videos.end() && videoIt->second.m_ownerUserId != "system")
		{
			LOGWARNING("AccountStore resource video id conflict ignored videoId=%s ownerUserId=%s filePath=%s",
			           videoId.c_str(),
			           videoIt->second.m_ownerUserId.c_str(),
			           files[i].c_str());
			continue;
		}

		VideoInfo video = videoIt == m_videos.end() ? VideoInfo() : videoIt->second;
		bool created = video.m_id.empty();
		video.m_id = videoId;
		video.m_ownerUserId = "system";
		video.m_mediaType = "video";
		video.m_filePath = CookHelper::normalizePathSeparators(files[i]);
		video.m_status = "published";
		if (video.m_createdAtSeconds <= 0)
		{
			video.m_createdAtSeconds = nowSeconds;
		}
		video.m_recipeIds.clear();
		const Recipe* recipe = findBaseRecipeNoLock(videoId);
		if (recipe != nullptr)
		{
			video.m_recipeIds.push_back(recipe->m_id);
			if (video.m_caption.empty())
			{
				video.m_caption = recipe->m_subtitle;
			}
		}
		if (video.m_caption.empty())
		{
			video.m_caption = "真实做菜短视频";
		}
		m_videos[videoId] = video;
		if (created)
		{
			LOGINFO("AccountStore resource video loaded videoId=%s filePath=%s recipeCount=%d",
			        videoId.c_str(),
			        video.m_filePath.c_str(),
			        static_cast<int32_t>(video.m_recipeIds.size()));
		}
	}

	std::string resourcePrefix = CookHelper::appendPathSeparator(resourceDirectory);
	for (std::map<std::string, VideoInfo>::iterator it = m_videos.begin(); it != m_videos.end();)
	{
		std::string filePath = resolveVideoFilePathNoLock(it->second.m_filePath);
		bool resourceVideo = it->second.m_ownerUserId == "system" &&
			it->second.m_status == "published" &&
			filePath.find(resourcePrefix) == 0;
		if (resourceVideo && foundIds.find(it->first) == foundIds.end())
		{
			LOGINFO("AccountStore resource video removed videoId=%s filePath=%s", it->first.c_str(), filePath.c_str());
			m_videos.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void AccountStore::pruneExpiredVideoSeenNoLock(UserAccount& account, int32_t nowSeconds) const
{
	if (g_config.m_videoSeenExpireDays <= 0)
	{
		account.m_watchedVideoAtSeconds.clear();
		return;
	}
	int64_t expireSeconds = static_cast<int64_t>(g_config.m_videoSeenExpireDays) * 24LL * 60LL * 60LL;
	for (std::map<std::string, int32_t>::iterator it = account.m_watchedVideoAtSeconds.begin(); it != account.m_watchedVideoAtSeconds.end();)
	{
		if (it->second <= 0 || static_cast<int64_t>(nowSeconds) - static_cast<int64_t>(it->second) >= expireSeconds)
		{
			account.m_watchedVideoAtSeconds.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

std::string AccountStore::videoRecommendScoreNoLock(const std::string& userId, const std::string& videoId, const std::string& seed) const
{
	return CookHelper::hashHex128(seed + "|" + userId + "|" + videoId);
}

void AccountStore::applyPersonalizationNoLock(const std::string& userId, Recipe& recipe) const
{
	std::string key = userId + "|" + recipe.m_id;
	std::map<std::string, PersonalizationConfig>::const_iterator it = m_personalizations.find(key);
	if (it == m_personalizations.end())
	{
		return;
	}

	const PersonalizationConfig& config = it->second;
	std::vector<CookTask> keptTasks;
	keptTasks.reserve(recipe.m_tasks.size() + config.m_addedTasks.size());
	for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
	{
		bool disabled = false;
		for (std::set<std::string>::const_iterator disabledIt = config.m_disabledTaskIds.begin();
			disabledIt != config.m_disabledTaskIds.end();
			++disabledIt)
		{
			if (CookHelper::taskIdMatches(recipe.m_tasks[i], *disabledIt))
			{
				disabled = true;
				break;
			}
		}
		if (!disabled)
		{
			keptTasks.push_back(recipe.m_tasks[i]);
		}
	}

	std::set<std::string> keptIds;
	for (size_t i = 0; i < keptTasks.size(); ++i)
	{
		for (std::map<std::string, int32_t>::const_iterator overrideIt = config.m_durationOverrides.begin();
			overrideIt != config.m_durationOverrides.end();
			++overrideIt)
		{
			if (overrideIt->second >= 0 && CookHelper::taskIdMatches(keptTasks[i], overrideIt->first))
			{
				keptTasks[i].m_durationSeconds = overrideIt->second;
			}
		}
		keptIds.insert(keptTasks[i].m_id);
	}

	for (size_t i = 0; i < keptTasks.size(); ++i)
	{
		std::vector<std::string> dependencies;
		for (size_t j = 0; j < keptTasks[i].m_dependencies.size(); ++j)
		{
			if (keptIds.find(keptTasks[i].m_dependencies[j]) != keptIds.end())
			{
				dependencies.push_back(keptTasks[i].m_dependencies[j]);
			}
		}
		keptTasks[i].m_dependencies = dependencies;
	}

	for (size_t i = 0; i < config.m_addedTasks.size(); ++i)
	{
		CookTask task = config.m_addedTasks[i];
		task.m_recipeId = recipe.m_id;
		if (task.m_shortId.empty())
		{
			task.m_shortId = CookHelper::taskShortId(task);
		}
		if (task.m_id.empty() || task.m_id.find(recipe.m_id + ".") != 0)
		{
			task.m_id = recipe.m_id + "." + (task.m_shortId.empty() ? ("custom_" + CStringManager::toStringInt32(static_cast<int32_t>(i))) : task.m_shortId);
		}
		keptTasks.push_back(task);
	}

	std::set<std::string> finalIds;
	for (size_t i = 0; i < keptTasks.size(); ++i)
	{
		finalIds.insert(keptTasks[i].m_id);
	}
	for (size_t i = 0; i < keptTasks.size(); ++i)
	{
		std::vector<std::string> dependencies;
		for (size_t j = 0; j < keptTasks[i].m_dependencies.size(); ++j)
		{
			std::string dependencyId = CookHelper::fullDependencyId(recipe.m_id, keptTasks[i].m_dependencies[j]);
			if (finalIds.find(dependencyId) != finalIds.end())
			{
				dependencies.push_back(dependencyId);
			}
		}
		keptTasks[i].m_dependencies = dependencies;
	}

	recipe.m_tasks = keptTasks;
	recipe.m_personalizationApplied = true;
	recipe.m_personalizationSummary = "已应用你的菜谱个性化设置";
}

std::string AccountStore::recipeToStorageJsonNoLock(const Recipe& recipe) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("schemaVersion", "1.0");
	CookServerHelper::addString(document, "id", recipe.m_id);
	CookServerHelper::addString(document, "title", recipe.m_title);
	CookServerHelper::addString(document, "category", recipe.m_category);
	CookServerHelper::addString(document, "subtitle", recipe.m_subtitle);
	CookServerHelper::addString(document, "difficulty", recipe.m_difficulty);
	CookServerHelper::addString(document, "coverColor", recipe.m_coverColor);
	CookServerHelper::addString(document, "author", recipe.m_author);
	CookServerHelper::addString(document, "authorUserId", recipe.m_authorUserId);
	CookServerHelper::addString(document, "priceType", recipe.m_priceType);
	CookServerHelper::addString(document, "status", recipe.m_status);
	CookServerHelper::addString(document, "currentRevisionId", recipe.m_currentRevisionId);
	CookServerHelper::addInt(document, "priceCoins", recipe.m_priceCoins);
	CookServerHelper::addInt(document, "priceAmount", recipe.m_priceAmount);
	CookServerHelper::addBool(document, "defaultOwned", recipe.m_defaultOwned);
	CookServerHelper::addBool(document, "systemRecipe", recipe.m_systemRecipe);
	CookServerHelper::addBool(document, "customRecipe", recipe.m_customRecipe);
	document.addStringArray("tags", recipe.m_tags);
	document.addStringArray("tools", recipe.m_tools);

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
	document.addValue("ingredients", ingredients);

	RapidJsonValue tasks;
	tasks.setArray();
	tasks.reserve(recipe.m_tasks.size());
	for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
	{
		const CookTask& task = recipe.m_tasks[i];
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "id", CookHelper::taskShortId(task));
		CookServerHelper::addString(item, "title", task.m_title);
		CookServerHelper::addString(item, "detail", task.m_detail);
		CookServerHelper::addString(item, "resource", task.m_resource);
		CookServerHelper::addString(item, "voiceText", task.m_voiceText);
		CookServerHelper::addString(item, "safetyLevel", task.m_safetyLevel);
		CookServerHelper::addString(item, "backgroundWaitMode", task.m_backgroundWaitMode);
		CookServerHelper::addInt(item, "durationSeconds", task.m_durationSeconds);
		CookServerHelper::addBool(item, "active", task.m_active);
		CookServerHelper::addString(item, "taskMode", task.m_active ? "foreground" : "background");
		CookServerHelper::addBool(item, "manualSkippable", task.m_manualSkippable);
		CookServerHelper::addBool(item, "canPause", task.m_canPause);
		CookServerHelper::addBool(item, "continuesDuringPause", task.m_continuesDuringPause);
		CookServerHelper::addBool(item, "canLeaveKitchen", task.m_canLeaveKitchen);

		RapidJsonValue dependencies;
		dependencies.setArray();
		dependencies.reserve(task.m_dependencies.size());
		for (size_t j = 0; j < task.m_dependencies.size(); ++j)
		{
			CookServerHelper::pushString(dependencies, CookHelper::shortDependencyId(recipe.m_id, task.m_dependencies[j]));
		}
		item.addValue("dependencies", dependencies);
		tasks.pushValue(item);
	}
	document.addValue("tasks", tasks);
	return document.toString();
}

std::string AccountStore::personalizationToStorageJsonNoLock(const PersonalizationConfig& config) const
{
	RapidJsonDocument document;
	document.setObject();
	document.addString("schemaVersion", "1.0");
	CookServerHelper::addString(document, "recipeId", config.m_recipeId);
	CookServerHelper::addInt(document, "updatedAtSeconds", config.m_updatedAtSeconds);

	RapidJsonValue disabled;
	disabled.setArray();
	disabled.reserve(config.m_disabledTaskIds.size());
	for (std::set<std::string>::const_iterator it = config.m_disabledTaskIds.begin(); it != config.m_disabledTaskIds.end(); ++it)
	{
		CookServerHelper::pushString(disabled, *it);
	}
	document.addValue("disabledTaskIds", disabled);

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
	document.addValue("durationOverrides", overrides);

	RapidJsonValue addedTasks;
	addedTasks.setArray();
	addedTasks.reserve(config.m_addedTasks.size());
	for (size_t i = 0; i < config.m_addedTasks.size(); ++i)
	{
		const CookTask& task = config.m_addedTasks[i];
		RapidJsonValue item;
		item.setObject();
		CookServerHelper::addString(item, "id", CookHelper::taskShortId(task));
		CookServerHelper::addString(item, "title", task.m_title);
		CookServerHelper::addString(item, "detail", task.m_detail);
		CookServerHelper::addString(item, "resource", task.m_resource);
		CookServerHelper::addString(item, "voiceText", task.m_voiceText);
		CookServerHelper::addString(item, "safetyLevel", task.m_safetyLevel);
		CookServerHelper::addString(item, "backgroundWaitMode", task.m_backgroundWaitMode);
		CookServerHelper::addInt(item, "durationSeconds", task.m_durationSeconds);
		CookServerHelper::addBool(item, "active", task.m_active);
		CookServerHelper::addString(item, "taskMode", task.m_active ? "foreground" : "background");
		CookServerHelper::addBool(item, "manualSkippable", task.m_manualSkippable);
		CookServerHelper::addBool(item, "canPause", task.m_canPause);
		CookServerHelper::addBool(item, "continuesDuringPause", task.m_continuesDuringPause);
		CookServerHelper::addBool(item, "canLeaveKitchen", task.m_canLeaveKitchen);
		RapidJsonValue dependencies;
		dependencies.setArray();
		dependencies.reserve(task.m_dependencies.size());
		for (size_t j = 0; j < task.m_dependencies.size(); ++j)
		{
			CookServerHelper::pushString(dependencies, CookHelper::shortDependencyId(config.m_recipeId, task.m_dependencies[j]));
		}
		item.addValue("dependencies", dependencies);
		addedTasks.pushValue(item);
	}
	document.addValue("addedTasks", addedTasks);
	return document.toString();
}

bool AccountStore::personalizationFromStorageJsonNoLock(const std::string& userId,
                                                        const std::string& recipeId,
                                                        const std::string& json,
                                                        PersonalizationConfig& config) const
{
	RapidJsonDocument document;
	if (!document.parse(json) || !document.isObject())
	{
		return false;
	}

	config = PersonalizationConfig();
	config.m_userId = userId;
	config.m_recipeId = recipeId;
	config.m_updatedAtSeconds = document.getIntOrDefault("updatedAtSeconds", nowSecondsNoLock());
	insertRecipeIdsNoLock(config.m_disabledTaskIds, document.getStringArrayOrEmpty("disabledTaskIds"));

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

	std::vector<RapidJsonValue> addedTaskValues = document.getArrayValueOrEmpty("addedTasks");
	for (size_t i = 0; i < addedTaskValues.size(); ++i)
	{
		CookTask task;
		task.m_shortId = addedTaskValues[i].getStringOrDefault("id", addedTaskValues[i].getStringOrDefault("shortId", ""));
		task.m_recipeId = recipeId;
		task.m_id = recipeId + "." + (task.m_shortId.empty() ? ("added_" + CStringManager::toStringInt32(static_cast<int32_t>(i))) : task.m_shortId);
		task.m_title = addedTaskValues[i].getStringOrDefault("title", "");
		task.m_detail = addedTaskValues[i].getStringOrDefault("detail", "");
		task.m_resource = addedTaskValues[i].getStringOrDefault("resource", "");
		task.m_voiceText = addedTaskValues[i].getStringOrDefault("voiceText", "");
		task.m_safetyLevel = addedTaskValues[i].getStringOrDefault("safetyLevel", "normal");
		task.m_backgroundWaitMode = addedTaskValues[i].getStringOrDefault("backgroundWaitMode", "");
		task.m_durationSeconds = addedTaskValues[i].getIntOrDefault("durationSeconds", 0);
		std::string taskMode = addedTaskValues[i].getStringOrDefault("taskMode", "");
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
			task.m_active = addedTaskValues[i].getBoolOrDefault("active", false);
		}
		task.m_manualSkippable = addedTaskValues[i].getBoolOrDefault("manualSkippable", task.m_active);
		task.m_canPause = addedTaskValues[i].getBoolOrDefault("canPause", false);
		task.m_continuesDuringPause = addedTaskValues[i].getBoolOrDefault("continuesDuringPause", false);
		task.m_canLeaveKitchen = addedTaskValues[i].getBoolOrDefault("canLeaveKitchen", false);
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
		std::vector<std::string> dependencies = addedTaskValues[i].getStringArrayOrEmpty("dependencies");
		for (size_t j = 0; j < dependencies.size(); ++j)
		{
			std::string dependencyId = CookHelper::fullDependencyId(recipeId, dependencies[j]);
			if (!dependencyId.empty())
			{
				task.m_dependencies.push_back(dependencyId);
			}
		}
		if (!task.m_title.empty() && task.m_durationSeconds >= 0)
		{
			if (task.m_voiceText.empty())
			{
				task.m_voiceText = task.m_title + "。" + task.m_detail;
			}
			config.m_addedTasks.push_back(task);
		}
	}
	return true;
}

void AccountStore::loadIfNeededNoLock()
{
	if (m_loaded)
	{
		return;
	}
	m_loaded = true;

	std::string storageStateText;
	std::istringstream storageInput;
	std::istream* input = nullptr;
	if (CookStorageService::instance().isEnabled() && CookStorageService::instance().loadAccountState(storageStateText))
	{
		storageInput.str(storageStateText);
		input = &storageInput;
	}
	else
	{
		LOGINFO("AccountStore state snapshot not found in Redis/MySQL, start with empty state storageEnabled=%d",
		        CookStorageService::instance().isEnabled() ? 1 : 0);
		return;
	}

	std::string line;
	while (input != nullptr && std::getline(*input, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
		{
			line.erase(line.size() - 1);
		}
		std::vector<std::string> parts = splitKeepEmptyNoLock(line, '\t');
		if (parts.empty())
		{
			continue;
		}

		if (parts[0] == "USER" && parts.size() >= 4)
		{
			UserAccount account;
			account.m_userId = parts[1];
			account.m_coins = static_cast<int32_t>(CStringManager::atoi64(parts[2].c_str()));
			insertRecipeIdsNoLock(account.m_purchasedRecipeIds, CookServerHelper::split(parts[3], ','));
			if (parts.size() >= 5)
			{
				insertRecipeIdsNoLock(account.m_favoriteRecipeIds, CookServerHelper::split(parts[4], ','));
			}
			if (parts.size() >= 6)
			{
				insertRecipeIdsNoLock(account.m_ownedRecipeIds, CookServerHelper::split(parts[5], ','));
			}
			account.m_account = parts.size() >= 7 ? parts[6] : account.m_userId;
			account.m_nickname = parts.size() >= 8 ? parts[7] : account.m_account;
			account.m_passwordHash = parts.size() >= 9 ? parts[8] : "";
			account.m_passwordSalt = parts.size() >= 10 ? parts[9] : "";
			account.m_yuanbao = parts.size() >= 11 ? static_cast<int32_t>(CStringManager::atoi64(parts[10].c_str())) : g_config.m_accountDefaultYuanbao;
			account.m_lastCheckinDate = parts.size() >= 12 ? parts[11] : "";
			if (parts.size() >= 13)
			{
				insertRecipeIdsNoLock(account.m_customRecipeIds, CookServerHelper::split(parts[12], ','));
			}
			if (parts.size() >= 14)
			{
				std::vector<std::string> following = CookServerHelper::split(parts[13], ',');
				account.m_followingUserIds.insert(following.begin(), following.end());
			}
			if (parts.size() >= 15)
			{
				std::vector<std::string> liked = CookServerHelper::split(parts[14], ',');
				for (size_t i = 0; i < liked.size(); ++i)
				{
					std::string targetKey = CookHelper::normalizeTargetKey(liked[i]);
					if (!targetKey.empty())
					{
						account.m_likedTargetIds.insert(targetKey);
					}
				}
			}
			if (parts.size() >= 16)
			{
				std::vector<std::string> favoriteTargets = CookServerHelper::split(parts[15], ',');
				for (size_t i = 0; i < favoriteTargets.size(); ++i)
				{
					std::string targetKey = CookHelper::normalizeTargetKey(favoriteTargets[i]);
					if (!targetKey.empty())
					{
						account.m_favoriteTargetIds.insert(targetKey);
					}
				}
			}
			account.m_createdAtSeconds = parts.size() >= 17 ? static_cast<int32_t>(CStringManager::atoi64(parts[16].c_str())) : 0;
			account.m_lastLoginAtSeconds = parts.size() >= 18 ? static_cast<int32_t>(CStringManager::atoi64(parts[17].c_str())) : 0;
			account.m_personalizationRecordEnabled = parts.size() >= 19 ? CookHelper::textToBool(parts[18], true) : true;
			if (!account.m_userId.empty())
			{
				refreshOwnedRecipesNoLock(account);
				m_accounts[account.m_userId] = account;
				if (!account.m_account.empty())
				{
					m_accountIndex[account.m_account] = account.m_userId;
				}
			}
		}
		else if (parts[0] == "SESSION" && parts.size() >= 4)
		{
			AuthSessionInfo session;
			session.m_tokenHash = parts[1];
			session.m_userId = parts[2];
			session.m_expireAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[3].c_str()));
			if (!session.m_tokenHash.empty() && !session.m_userId.empty())
			{
				m_sessions[session.m_tokenHash] = session;
			}
		}
		else if (parts[0] == "AD" && parts.size() >= 6)
		{
			AdRewardInfo reward;
			reward.m_adToken = parts[1];
			reward.m_userId = parts[2];
			reward.m_createdAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[3].c_str()));
			reward.m_rewardCoins = static_cast<int32_t>(CStringManager::atoi64(parts[4].c_str()));
			reward.m_finished = CookHelper::textToBool(parts[5], false);
			if (!reward.m_adToken.empty())
			{
				m_adRewards[reward.m_adToken] = reward;
			}
		}
		else if (parts[0] == "CUSTOM" && parts.size() >= 3)
		{
			Recipe recipe;
			std::string errorMessage;
			if (CookRecipeConfig::loadRecipeFromJson(parts[2], recipe, &errorMessage))
			{
				recipe.m_authorUserId = parts[1];
				recipe.m_customRecipe = true;
				recipe.m_systemRecipe = false;
				m_customRecipes[recipe.m_id] = recipe;
			}
		}
		else if (parts[0] == "COMMENT" && parts.size() >= 9)
		{
			CommentInfo comment;
			comment.m_id = parts[1];
			comment.m_targetType = parts[2];
			comment.m_targetId = parts[3];
			comment.m_userId = parts[4];
			comment.m_nickname = parts[5];
			comment.m_content = parts[6];
			comment.m_score = static_cast<int32_t>(CStringManager::atoi64(parts[7].c_str()));
			comment.m_createdAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[8].c_str()));
			m_comments.push_back(comment);
		}
		else if (parts[0] == "MESSAGE" && parts.size() >= 9)
		{
			MessageInfo message;
			message.m_id = parts[1];
			message.m_fromUserId = parts[2];
			message.m_toUserId = parts[3];
			message.m_type = parts[4];
			message.m_title = parts[5];
			message.m_text = parts[6];
			message.m_createdAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[7].c_str()));
			message.m_read = CookHelper::textToBool(parts[8], false);
			m_messages.push_back(message);
		}
		else if (parts[0] == "VIDEO" && parts.size() >= 9)
		{
			VideoInfo video;
			video.m_id = parts[1];
			video.m_ownerUserId = parts[2];
			video.m_mediaType = parts[3];
			video.m_filePath = parts[4];
			video.m_caption = parts[5];
			video.m_recipeIds = CookServerHelper::split(parts[6], ',');
			video.m_status = parts[7];
			video.m_createdAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[8].c_str()));
			if (!video.m_id.empty())
			{
				m_videos[video.m_id] = video;
			}
		}
		else if (parts[0] == "VIDEO_SEEN" && parts.size() >= 4)
		{
			std::string userId = parts[1];
			std::string videoId = CookHelper::normalizeVideoId(parts[2]);
			int32_t seenAtSeconds = static_cast<int32_t>(CStringManager::atoi64(parts[3].c_str()));
			if (!userId.empty() && !videoId.empty() && seenAtSeconds > 0)
			{
				std::map<std::string, UserAccount>::iterator accountIt = m_accounts.find(userId);
				if (accountIt != m_accounts.end())
				{
					accountIt->second.m_watchedVideoAtSeconds[videoId] = seenAtSeconds;
				}
			}
		}
		else if (parts[0] == "PERS" && parts.size() >= 4)
		{
			PersonalizationConfig config;
			if (personalizationFromStorageJsonNoLock(parts[1], parts[2], parts[3], config))
			{
				m_personalizations[parts[1] + "|" + parts[2]] = config;
			}
		}
		else if (parts[0] == "SHARE" && parts.size() >= 5)
		{
			PersonalizationConfig config;
			if (personalizationFromStorageJsonNoLock(parts[2], parts[3], parts[4], config))
			{
				m_sharedPersonalizations[parts[1]] = config;
			}
		}
		else if (parts[0] == "IDEMP" && parts.size() >= 2)
		{
			m_idempotencyKeys.insert(parts[1]);
		}
	}
	int32_t likedTargetCount = 0;
	int32_t favoriteTargetCount = 0;
	int32_t favoriteRecipeCount = 0;
	for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
	{
		likedTargetCount += static_cast<int32_t>(it->second.m_likedTargetIds.size());
		favoriteTargetCount += static_cast<int32_t>(it->second.m_favoriteTargetIds.size());
		favoriteRecipeCount += static_cast<int32_t>(it->second.m_favoriteRecipeIds.size());
	}
	LOGINFO("AccountStore state loaded from Redis/MySQL accountCount=%d customRecipeCount=%d personalizationCount=%d likedTargetCount=%d favoriteTargetCount=%d favoriteRecipeCount=%d",
	        static_cast<int32_t>(m_accounts.size()),
	        static_cast<int32_t>(m_customRecipes.size()),
	        static_cast<int32_t>(m_personalizations.size()),
	        likedTargetCount,
	        favoriteTargetCount,
	        favoriteRecipeCount);
}

void AccountStore::saveNoLock()
{
	if (!CookStorageService::instance().isEnabled())
	{
		LOGERROR("AccountStore save failed, Redis/MySQL storage service is disabled.");
		return;
	}
	std::ostringstream memoryOutput;
	std::ostream& output = memoryOutput;

	for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
	{
		const UserAccount& account = it->second;
		output << "USER"
			<< "\t" << safeFieldNoLock(account.m_userId)
			<< "\t" << account.m_coins
			<< "\t" << CookServerHelper::joinSet(account.m_purchasedRecipeIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_favoriteRecipeIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_ownedRecipeIds, ',')
			<< "\t" << safeFieldNoLock(account.m_account)
			<< "\t" << safeFieldNoLock(account.m_nickname)
			<< "\t" << account.m_passwordHash
			<< "\t" << account.m_passwordSalt
			<< "\t" << account.m_yuanbao
			<< "\t" << account.m_lastCheckinDate
			<< "\t" << CookServerHelper::joinSet(account.m_customRecipeIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_followingUserIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_likedTargetIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_favoriteTargetIds, ',')
			<< "\t" << account.m_createdAtSeconds
			<< "\t" << account.m_lastLoginAtSeconds
			<< "\t" << CookHelper::boolToText(account.m_personalizationRecordEnabled)
			<< "\n";
	}
	for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
	{
		const UserAccount& account = it->second;
		for (std::map<std::string, int32_t>::const_iterator seenIt = account.m_watchedVideoAtSeconds.begin();
			seenIt != account.m_watchedVideoAtSeconds.end();
			++seenIt)
		{
			output << "VIDEO_SEEN"
				<< "\t" << safeFieldNoLock(account.m_userId)
				<< "\t" << safeFieldNoLock(seenIt->first)
				<< "\t" << seenIt->second
				<< "\n";
		}
	}

	for (std::map<std::string, AuthSessionInfo>::const_iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
	{
		output << "SESSION"
			<< "\t" << it->second.m_tokenHash
			<< "\t" << safeFieldNoLock(it->second.m_userId)
			<< "\t" << it->second.m_expireAtSeconds
			<< "\n";
	}
	for (std::map<std::string, AdRewardInfo>::const_iterator it = m_adRewards.begin(); it != m_adRewards.end(); ++it)
	{
		output << "AD"
			<< "\t" << safeFieldNoLock(it->second.m_adToken)
			<< "\t" << safeFieldNoLock(it->second.m_userId)
			<< "\t" << it->second.m_createdAtSeconds
			<< "\t" << it->second.m_rewardCoins
			<< "\t" << CookHelper::boolToText(it->second.m_finished)
			<< "\n";
	}
	for (std::map<std::string, Recipe>::const_iterator it = m_customRecipes.begin(); it != m_customRecipes.end(); ++it)
	{
		output << "CUSTOM"
			<< "\t" << safeFieldNoLock(it->second.m_authorUserId)
			<< "\t" << safeFieldNoLock(recipeToStorageJsonNoLock(it->second))
			<< "\n";
	}
	for (size_t i = 0; i < m_comments.size(); ++i)
	{
		output << "COMMENT"
			<< "\t" << safeFieldNoLock(m_comments[i].m_id)
			<< "\t" << safeFieldNoLock(m_comments[i].m_targetType)
			<< "\t" << safeFieldNoLock(m_comments[i].m_targetId)
			<< "\t" << safeFieldNoLock(m_comments[i].m_userId)
			<< "\t" << safeFieldNoLock(m_comments[i].m_nickname)
			<< "\t" << safeFieldNoLock(m_comments[i].m_content)
			<< "\t" << m_comments[i].m_score
			<< "\t" << m_comments[i].m_createdAtSeconds
			<< "\n";
	}
	for (size_t i = 0; i < m_messages.size(); ++i)
	{
		output << "MESSAGE"
			<< "\t" << safeFieldNoLock(m_messages[i].m_id)
			<< "\t" << safeFieldNoLock(m_messages[i].m_fromUserId)
			<< "\t" << safeFieldNoLock(m_messages[i].m_toUserId)
			<< "\t" << safeFieldNoLock(m_messages[i].m_type)
			<< "\t" << safeFieldNoLock(m_messages[i].m_title)
			<< "\t" << safeFieldNoLock(m_messages[i].m_text)
			<< "\t" << m_messages[i].m_createdAtSeconds
			<< "\t" << CookHelper::boolToText(m_messages[i].m_read)
			<< "\n";
	}
	for (std::map<std::string, VideoInfo>::const_iterator it = m_videos.begin(); it != m_videos.end(); ++it)
	{
		output << "VIDEO"
			<< "\t" << safeFieldNoLock(it->second.m_id)
			<< "\t" << safeFieldNoLock(it->second.m_ownerUserId)
			<< "\t" << safeFieldNoLock(it->second.m_mediaType)
			<< "\t" << safeFieldNoLock(it->second.m_filePath)
			<< "\t" << safeFieldNoLock(it->second.m_caption)
			<< "\t" << safeFieldNoLock(CookHelper::joinVector(it->second.m_recipeIds, ','))
			<< "\t" << safeFieldNoLock(it->second.m_status)
			<< "\t" << it->second.m_createdAtSeconds
			<< "\n";
	}
	for (std::map<std::string, PersonalizationConfig>::const_iterator it = m_personalizations.begin(); it != m_personalizations.end(); ++it)
	{
		output << "PERS"
			<< "\t" << safeFieldNoLock(it->second.m_userId)
			<< "\t" << safeFieldNoLock(it->second.m_recipeId)
			<< "\t" << safeFieldNoLock(personalizationToStorageJsonNoLock(it->second))
			<< "\n";
	}
	for (std::map<std::string, PersonalizationConfig>::const_iterator it = m_sharedPersonalizations.begin(); it != m_sharedPersonalizations.end(); ++it)
	{
		output << "SHARE"
			<< "\t" << safeFieldNoLock(it->first)
			<< "\t" << safeFieldNoLock(it->second.m_userId)
			<< "\t" << safeFieldNoLock(it->second.m_recipeId)
			<< "\t" << safeFieldNoLock(personalizationToStorageJsonNoLock(it->second))
			<< "\n";
	}
	for (std::set<std::string>::const_iterator it = m_idempotencyKeys.begin(); it != m_idempotencyKeys.end(); ++it)
	{
		output << "IDEMP" << "\t" << safeFieldNoLock(*it) << "\n";
	}

	output.flush();
	if (!output.good())
	{
		LOGERROR("AccountStore save failed, write memory stream failed.");
		return;
	}
	std::string stateText = memoryOutput.str();
	if (!CookStorageService::instance().saveAccountStateAsync(stateText))
	{
		LOGERROR("AccountStore save failed, storage service rejected accountCount=%d bytes=%d",
		         static_cast<int32_t>(m_accounts.size()),
		         static_cast<int32_t>(stateText.size()));
		return;
	}
	LOGDEBUG("AccountStore state saved to Redis/MySQL accountCount=%d bytes=%d",
	         static_cast<int32_t>(m_accounts.size()),
	         static_cast<int32_t>(stateText.size()));
}

UserAccount& AccountStore::ensureNoLock(const std::string& userId)
{
	loadIfNeededNoLock();
	std::string id = normalizeUserId(userId);
	std::map<std::string, UserAccount>::iterator it = m_accounts.find(id);
	if (it == m_accounts.end())
	{
		UserAccount account;
		account.m_userId = id;
		account.m_account = id;
		account.m_nickname = id;
		account.m_coins = g_config.m_accountDefaultCoins;
		account.m_yuanbao = g_config.m_accountDefaultYuanbao;
		account.m_createdAtSeconds = nowSecondsNoLock();
		account.m_lastLoginAtSeconds = account.m_createdAtSeconds;
		refreshOwnedRecipesNoLock(account);
		m_accounts[id] = account;
		m_accountIndex[account.m_account] = account.m_userId;
		addMessageNoLock("system", id, "system", "欢迎使用会做饭", "第一期账号已创建，默认菜谱已经加入菜单。");
		saveNoLock();
		LOGINFO("AccountStore create compatibility account userId=%s", id.c_str());
	}
	else
	{
		refreshOwnedRecipesNoLock(it->second);
	}
	return m_accounts[id];
}

void AccountStore::createSessionNoLock(const std::string& userId, std::string& token, int32_t& expireInSeconds)
{
	expireInSeconds = g_config.m_authTokenExpireSeconds;
	token = makeIdNoLock("tok");
	AuthSessionInfo session;
	session.m_tokenHash = hashTextNoLock(token);
	session.m_userId = userId;
	session.m_expireAtSeconds = nowSecondsNoLock() + expireInSeconds;
	m_sessions[session.m_tokenHash] = session;
}

bool AccountStore::registerAccount(const std::string& accountName,
                                   const std::string& password,
                                   const std::string& nickname,
                                   UserAccount& account,
                                   std::string& token,
                                   int32_t& expireInSeconds,
                                   std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string realAccountName = normalizeAccountNameNoLock(accountName);
	if (realAccountName.size() < 3 || realAccountName.size() > 64)
	{
		message = "账号长度需要为3-64个字符";
		return false;
	}
	if (password.size() < 4)
	{
		message = "密码至少需要4个字符";
		return false;
	}
	if (m_accountIndex.find(realAccountName) != m_accountIndex.end())
	{
		message = "账号已存在";
		return false;
	}

	UserAccount newAccount;
	newAccount.m_userId = "u_" + hashTextNoLock(realAccountName + "|" + makeIdNoLock("user")).substr(0, 16);
	while (m_accounts.find(newAccount.m_userId) != m_accounts.end())
	{
		newAccount.m_userId = makeIdNoLock("user");
	}
	newAccount.m_account = realAccountName;
	newAccount.m_nickname = nickname.empty() ? realAccountName : nickname;
	newAccount.m_passwordSalt = makeIdNoLock("salt");
	newAccount.m_passwordHash = hashTextNoLock(password + "|" + newAccount.m_passwordSalt);
	newAccount.m_coins = g_config.m_accountDefaultCoins;
	newAccount.m_yuanbao = g_config.m_accountDefaultYuanbao;
	newAccount.m_createdAtSeconds = nowSecondsNoLock();
	newAccount.m_lastLoginAtSeconds = newAccount.m_createdAtSeconds;
	refreshOwnedRecipesNoLock(newAccount);

	m_accounts[newAccount.m_userId] = newAccount;
	m_accountIndex[newAccount.m_account] = newAccount.m_userId;
	createSessionNoLock(newAccount.m_userId, token, expireInSeconds);
	addMessageNoLock("system", newAccount.m_userId, "system", "注册成功", "默认免费菜谱已加入菜单，可以先从糖醋排骨配米饭开始。");
	account = m_accounts[newAccount.m_userId];
	saveNoLock();
	message = "注册成功";
	return true;
}

bool AccountStore::loginAccount(const std::string& accountName,
                                const std::string& password,
                                UserAccount& account,
                                std::string& token,
                                int32_t& expireInSeconds,
                                std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string realAccountName = normalizeAccountNameNoLock(accountName);
	std::map<std::string, std::string>::const_iterator indexIt = m_accountIndex.find(realAccountName);
	if (indexIt == m_accountIndex.end())
	{
		message = "账号或密码错误";
		return false;
	}
	UserAccount& current = ensureNoLock(indexIt->second);
	if (current.m_passwordHash.empty() || current.m_passwordHash != hashTextNoLock(password + "|" + current.m_passwordSalt))
	{
		message = "账号或密码错误";
		return false;
	}
	current.m_lastLoginAtSeconds = nowSecondsNoLock();
	createSessionNoLock(current.m_userId, token, expireInSeconds);
	account = current;
	saveNoLock();
	message = "登录成功";
	return true;
}

bool AccountStore::validateToken(const std::string& token, std::string& userId, UserAccount& account, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	if (token.empty())
	{
		message = "缺少登录token";
		return false;
	}
	std::string tokenHash = hashTextNoLock(token);
	std::map<std::string, AuthSessionInfo>::iterator sessionIt = m_sessions.find(tokenHash);
	if (sessionIt == m_sessions.end())
	{
		message = "登录已失效";
		return false;
	}
	if (sessionIt->second.m_expireAtSeconds <= nowSecondsNoLock())
	{
		m_sessions.erase(sessionIt);
		saveNoLock();
		message = "登录已过期";
		return false;
	}
	userId = sessionIt->second.m_userId;
	account = ensureNoLock(userId);
	return true;
}

bool AccountStore::logout(const std::string& token)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string tokenHash = hashTextNoLock(token);
	std::map<std::string, AuthSessionInfo>::iterator it = m_sessions.find(tokenHash);
	if (it == m_sessions.end())
	{
		return false;
	}
	m_sessions.erase(it);
	saveNoLock();
	return true;
}

UserAccount AccountStore::getAccount(const std::string& userId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return ensureNoLock(userId);
}

UserAccount AccountStore::recharge(const std::string& userId, int32_t coins, bool* ok, std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	if (coins <= 0 || coins > g_config.m_accountMaxRechargeCoins)
	{
		if (ok)
		{
			*ok = false;
		}
		if (message)
		{
			*message = "coins must be 1-" + CStringManager::toStringInt32(g_config.m_accountMaxRechargeCoins);
		}
		return account;
	}
	account.m_coins += coins;
	saveNoLock();
	if (ok)
	{
		*ok = true;
	}
	if (message)
	{
		*message = "充值成功";
	}
	return account;
}

UserAccount AccountStore::joinRecipe(const std::string& userId,
                                     const std::string& recipeId,
                                     const std::string& idempotencyKey,
                                     bool* ok,
                                     bool* alreadyOwned,
                                     std::string* message,
                                     Recipe* recipe)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realRecipeId = normalizeRecipeIdNoLock(recipeId);
	const Recipe* baseRecipe = findBaseRecipeNoLock(realRecipeId);
	if (baseRecipe == nullptr)
	{
		if (ok)
		{
			*ok = false;
		}
		if (alreadyOwned)
		{
			*alreadyOwned = false;
		}
		if (message)
		{
			*message = "菜谱不存在";
		}
		return account;
	}

	std::string idemKey = idempotencyKey.empty() ? "" : (account.m_userId + "|join|" + idempotencyKey);
	if (!idemKey.empty() && m_idempotencyKeys.find(idemKey) != m_idempotencyKeys.end())
	{
		Recipe snapshot = *baseRecipe;
		if (AccountStore::isRecipeOwned(snapshot, account))
		{
			applyPersonalizationNoLock(account.m_userId, snapshot);
		}
		if (recipe)
		{
			*recipe = snapshot;
		}
		if (ok)
		{
			*ok = true;
		}
		if (alreadyOwned)
		{
			*alreadyOwned = true;
		}
		if (message)
		{
			*message = "请求已处理";
		}
		return account;
	}

	bool ownedBefore = AccountStore::isRecipeOwned(*baseRecipe, account);
	if (ownedBefore)
	{
		if (!idemKey.empty())
		{
			m_idempotencyKeys.insert(idemKey);
		}
		Recipe snapshot = *baseRecipe;
		applyPersonalizationNoLock(account.m_userId, snapshot);
		if (recipe)
		{
			*recipe = snapshot;
		}
		if (ok)
		{
			*ok = true;
		}
		if (alreadyOwned)
		{
			*alreadyOwned = true;
		}
		if (message)
		{
			*message = "已经拥有该菜谱";
		}
		saveNoLock();
		return account;
	}

	std::string priceType = baseRecipe->m_priceType.empty() ? (baseRecipe->m_priceCoins > 0 ? "coin" : "free") : baseRecipe->m_priceType;
	int32_t priceAmount = baseRecipe->m_priceAmount > 0 ? baseRecipe->m_priceAmount : baseRecipe->m_priceCoins;
	if (priceType == "free" || priceAmount <= 0)
	{
		account.m_ownedRecipeIds.insert(baseRecipe->m_id);
	}
	else if (priceType == "yuanbao")
	{
		if (account.m_yuanbao < priceAmount)
		{
			if (ok)
			{
				*ok = false;
			}
			if (alreadyOwned)
			{
				*alreadyOwned = false;
			}
			if (message)
			{
				*message = "元宝不足";
			}
			return account;
		}
		account.m_yuanbao -= priceAmount;
		account.m_purchasedRecipeIds.insert(baseRecipe->m_id);
		account.m_ownedRecipeIds.insert(baseRecipe->m_id);
	}
	else if (priceType == "coin")
	{
		if (account.m_coins < priceAmount)
		{
			if (ok)
			{
				*ok = false;
			}
			if (alreadyOwned)
			{
				*alreadyOwned = false;
			}
			if (message)
			{
				*message = "金币不足，请先签到或看广告获取金币";
			}
			return account;
		}
		account.m_coins -= priceAmount;
		account.m_purchasedRecipeIds.insert(baseRecipe->m_id);
		account.m_ownedRecipeIds.insert(baseRecipe->m_id);
	}
	else
	{
		if (ok)
		{
			*ok = false;
		}
		if (alreadyOwned)
		{
			*alreadyOwned = false;
		}
		if (message)
		{
			*message = "不支持的价格类型";
		}
		return account;
	}

	if (!idemKey.empty())
	{
		m_idempotencyKeys.insert(idemKey);
	}
	Recipe snapshot = *baseRecipe;
	applyPersonalizationNoLock(account.m_userId, snapshot);
	if (recipe)
	{
		*recipe = snapshot;
	}
	if (ok)
	{
		*ok = true;
	}
	if (alreadyOwned)
	{
		*alreadyOwned = false;
	}
	if (message)
	{
		*message = "已加入菜单";
	}
	saveNoLock();
	return account;
}

UserAccount AccountStore::purchaseRecipe(const std::string& userId, const std::string& recipeId, bool* ok, std::string* message)
{
	bool alreadyOwned = false;
	Recipe recipe;
	return joinRecipe(userId, recipeId, "", ok, &alreadyOwned, message, &recipe);
}

UserAccount AccountStore::checkin(const std::string& userId, bool* ok, bool* alreadyCheckedIn, std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string today = todayTextNoLock();
	if (account.m_lastCheckinDate == today)
	{
		if (ok)
		{
			*ok = true;
		}
		if (alreadyCheckedIn)
		{
			*alreadyCheckedIn = true;
		}
		if (message)
		{
			*message = "今天已经签到";
		}
		return account;
	}
	account.m_lastCheckinDate = today;
	account.m_coins += g_config.m_checkinRewardCoins;
	saveNoLock();
	if (ok)
	{
		*ok = true;
	}
	if (alreadyCheckedIn)
	{
		*alreadyCheckedIn = false;
	}
	if (message)
	{
		*message = "签到成功，获得" + CStringManager::toStringInt32(g_config.m_checkinRewardCoins) + "金币";
	}
	return account;
}

UserAccount AccountStore::startAdReward(const std::string& userId, std::string& adToken, int32_t& rewardCoins, int32_t& requiredSeconds)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	AdRewardInfo reward;
	reward.m_adToken = makeIdNoLock("ad");
	reward.m_userId = account.m_userId;
	reward.m_createdAtSeconds = nowSecondsNoLock();
	reward.m_rewardCoins = g_config.m_adRewardCoins;
	reward.m_finished = false;
	m_adRewards[reward.m_adToken] = reward;
	adToken = reward.m_adToken;
	rewardCoins = reward.m_rewardCoins;
	requiredSeconds = g_config.m_adRequiredSeconds;
	saveNoLock();
	return account;
}

UserAccount AccountStore::finishAdReward(const std::string& userId,
                                         const std::string& adToken,
                                         int32_t debugElapsedSeconds,
                                         bool* ok,
                                         bool* alreadyRewarded,
                                         std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::map<std::string, AdRewardInfo>::iterator rewardIt = m_adRewards.find(adToken);
	if (rewardIt == m_adRewards.end() || rewardIt->second.m_userId != account.m_userId)
	{
		if (ok)
		{
			*ok = false;
		}
		if (alreadyRewarded)
		{
			*alreadyRewarded = false;
		}
		if (message)
		{
			*message = "广告奖励不存在";
		}
		return account;
	}
	if (rewardIt->second.m_finished)
	{
		if (ok)
		{
			*ok = true;
		}
		if (alreadyRewarded)
		{
			*alreadyRewarded = true;
		}
		if (message)
		{
			*message = "奖励已领取";
		}
		return account;
	}
	int32_t elapsed = debugElapsedSeconds >= 0 ? debugElapsedSeconds : (nowSecondsNoLock() - rewardIt->second.m_createdAtSeconds);
	if (elapsed < g_config.m_adRequiredSeconds)
	{
		if (ok)
		{
			*ok = false;
		}
		if (alreadyRewarded)
		{
			*alreadyRewarded = false;
		}
		if (message)
		{
			*message = "广告观看时间不足";
		}
		return account;
	}
	rewardIt->second.m_finished = true;
	account.m_coins += rewardIt->second.m_rewardCoins;
	saveNoLock();
	if (ok)
	{
		*ok = true;
	}
	if (alreadyRewarded)
	{
		*alreadyRewarded = false;
	}
	if (message)
	{
		*message = "广告奖励领取成功";
	}
	return account;
}

UserAccount AccountStore::toggleFavorite(const std::string& userId,
                                         const std::string& targetType,
                                         const std::string& targetId,
                                         bool* ok,
                                         bool* favorite,
                                         std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realType = targetType.empty() ? "recipe" : targetType;
	std::string realTargetId = realType == "recipe" ? normalizeRecipeIdNoLock(targetId) : CookHelper::normalizeVideoId(targetId);
	if (realTargetId.empty())
	{
		if (ok)
		{
			*ok = false;
		}
		if (favorite)
		{
			*favorite = false;
		}
		if (message)
		{
			*message = "目标不能为空";
		}
		return account;
	}
	if (realType == "recipe" && findBaseRecipeNoLock(realTargetId) == nullptr)
	{
		if (ok)
		{
			*ok = false;
		}
		if (favorite)
		{
			*favorite = false;
		}
		if (message)
		{
			*message = "菜谱不存在";
		}
		return account;
	}
	if (realType == "video")
	{
		syncResourceVideosNoLock(false);
		if (m_videos.find(realTargetId) == m_videos.end())
		{
			if (ok)
			{
				*ok = false;
			}
			if (favorite)
			{
				*favorite = false;
			}
			if (message)
			{
				*message = "视频不存在";
			}
			return account;
		}
	}

	std::string key = targetKeyNoLock(realType, realTargetId);
	bool currentFavorite = account.m_favoriteTargetIds.find(key) == account.m_favoriteTargetIds.end();
	if (currentFavorite)
	{
		account.m_favoriteTargetIds.insert(key);
		if (realType == "recipe")
		{
			account.m_favoriteRecipeIds.insert(realTargetId);
		}
	}
	else
	{
		account.m_favoriteTargetIds.erase(key);
		if (realType == "recipe")
		{
			account.m_favoriteRecipeIds.erase(realTargetId);
		}
	}
	saveNoLock();
	if (ok)
	{
		*ok = true;
	}
	if (favorite)
	{
		*favorite = currentFavorite;
	}
	if (message)
	{
		*message = "收藏状态已更新";
	}
	return account;
}

UserAccount AccountStore::toggleFavorite(const std::string& userId, const std::string& recipeId, bool* ok, bool* favorite, std::string* message)
{
	return toggleFavorite(userId, "recipe", recipeId, ok, favorite, message);
}

bool AccountStore::toggleLike(const std::string& userId, const std::string& targetType, const std::string& targetId, bool& liked)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realType = targetType.empty() ? "video" : targetType;
	std::string realTargetId = realType == "recipe" ? normalizeRecipeIdNoLock(targetId) : CookHelper::normalizeVideoId(targetId);
	if (realTargetId.empty())
	{
		liked = false;
		return false;
	}
	if (realType == "recipe" && findBaseRecipeNoLock(realTargetId) == nullptr)
	{
		liked = false;
		return false;
	}
	if (realType == "video")
	{
		syncResourceVideosNoLock(false);
		if (m_videos.find(realTargetId) == m_videos.end())
		{
			liked = false;
			return false;
		}
	}

	std::string key = targetKeyNoLock(realType, realTargetId);
	std::set<std::string>::iterator it = account.m_likedTargetIds.find(key);
	if (it == account.m_likedTargetIds.end())
	{
		account.m_likedTargetIds.insert(key);
		liked = true;
	}
	else
	{
		account.m_likedTargetIds.erase(it);
		liked = false;
	}
	saveNoLock();
	return true;
}

void AccountStore::addMessageNoLock(const std::string& fromUserId,
                                    const std::string& toUserId,
                                    const std::string& type,
                                    const std::string& title,
                                    const std::string& text)
{
	if (toUserId.empty())
	{
		return;
	}
	MessageInfo message;
	message.m_id = makeIdNoLock("msg");
	message.m_fromUserId = fromUserId.empty() ? "system" : fromUserId;
	message.m_toUserId = toUserId;
	message.m_type = type.empty() ? "system" : type;
	message.m_title = title;
	message.m_text = text;
	message.m_createdAtSeconds = nowSecondsNoLock();
	message.m_read = false;
	m_messages.push_back(message);
}

bool AccountStore::toggleFollow(const std::string& userId, const std::string& targetUserId, bool& following, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string targetInput = normalizeAccountNameNoLock(targetUserId);
	if (targetInput.empty())
	{
		following = false;
		message = "关注目标不能为空";
		return false;
	}
	std::string targetId = normalizeUserId(targetInput);
	if (targetId.empty() || targetId == account.m_userId)
	{
		following = false;
		message = "不能关注自己";
		return false;
	}
	ensureNoLock(targetId);
	std::set<std::string>::iterator it = account.m_followingUserIds.find(targetId);
	if (it == account.m_followingUserIds.end())
	{
		account.m_followingUserIds.insert(targetId);
		following = true;
		message = "已关注";
		addMessageNoLock(account.m_userId, targetId, "follow", "有人关注了你", account.m_nickname + " 关注了你");
	}
	else
	{
		account.m_followingUserIds.erase(it);
		following = false;
		message = "已取消关注";
	}
	saveNoLock();
	return true;
}

bool AccountStore::createComment(const std::string& userId,
                                 const std::string& targetType,
                                 const std::string& targetId,
                                 const std::string& content,
                                 int32_t score,
                                 CommentInfo& comment,
                                 std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realType = targetType.empty() ? "recipe" : targetType;
	std::string realTargetId = realType == "recipe" ? normalizeRecipeIdNoLock(targetId) : CookHelper::normalizeVideoId(targetId);
	if (content.empty())
	{
		message = "评论内容不能为空";
		return false;
	}
	if (realType == "recipe")
	{
		const Recipe* recipe = findBaseRecipeNoLock(realTargetId);
		if (recipe == nullptr)
		{
			message = "菜谱不存在";
			return false;
		}
		if (!AccountStore::isRecipeOwned(*recipe, account))
		{
			message = "购买或加入菜谱后才能评论";
			return false;
		}
		if (!recipe->m_authorUserId.empty() && recipe->m_authorUserId != account.m_userId)
		{
			addMessageNoLock(account.m_userId, recipe->m_authorUserId, "comment", "菜谱有新评论", account.m_nickname + " 评论了《" + recipe->m_title + "》");
		}
	}
	else if (realType == "video" && m_videos.find(realTargetId) == m_videos.end())
	{
		syncResourceVideosNoLock(false);
		if (m_videos.find(realTargetId) == m_videos.end())
		{
			message = "视频不存在";
			return false;
		}
	}

	comment = CommentInfo();
	comment.m_id = makeIdNoLock("cmt");
	comment.m_targetType = realType;
	comment.m_targetId = realTargetId;
	comment.m_userId = account.m_userId;
	comment.m_nickname = account.m_nickname;
	comment.m_content = content;
	comment.m_score = score < 0 ? 0 : (score > 5 ? 5 : score);
	comment.m_createdAtSeconds = nowSecondsNoLock();
	m_comments.push_back(comment);
	saveNoLock();
	message = "评论成功";
	return true;
}

std::vector<CommentInfo> AccountStore::listComments(const std::string& targetType, const std::string& targetId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string realType = targetType.empty() ? "recipe" : targetType;
	std::string realTargetId = realType == "recipe" ? normalizeRecipeIdNoLock(targetId) : targetId;
	std::vector<CommentInfo> result;
	for (size_t i = 0; i < m_comments.size(); ++i)
	{
		if (m_comments[i].m_targetType == realType && m_comments[i].m_targetId == realTargetId)
		{
			result.push_back(m_comments[i]);
		}
	}
	return result;
}

std::vector<MessageInfo> AccountStore::listMessages(const std::string& userId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::vector<MessageInfo> result;
	for (size_t i = 0; i < m_messages.size(); ++i)
	{
		if (m_messages[i].m_toUserId == account.m_userId)
		{
			result.push_back(m_messages[i]);
		}
	}
	return result;
}

bool AccountStore::uploadVideo(const std::string& userId,
                               const std::string& mediaType,
                               const std::string& filePath,
                               int32_t fileSizeBytes,
                               VideoInfo& video,
                               std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	if (fileSizeBytes > 0 && static_cast<int64_t>(fileSizeBytes) > g_config.m_maxVideoBytes)
	{
		message = "视频文件过大";
		return false;
	}
	video = VideoInfo();
	video.m_id = makeIdNoLock("video");
	video.m_ownerUserId = account.m_userId;
	video.m_mediaType = mediaType.empty() ? "video" : mediaType;
	video.m_filePath = filePath.empty() ? (g_config.m_videoStorageDirectoryName + "/" + video.m_id) : filePath;
	video.m_status = "uploaded";
	video.m_createdAtSeconds = nowSecondsNoLock();
	m_videos[video.m_id] = video;
	saveNoLock();
	message = "上传登记成功";
	return true;
}

bool AccountStore::publishVideo(const std::string& userId,
                                const std::string& videoId,
                                const std::string& caption,
                                const std::vector<std::string>& recipeIds,
                                VideoInfo& video,
                                std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	if (recipeIds.empty())
	{
		message = "至少挂载一个菜谱";
		return false;
	}
	for (size_t i = 0; i < recipeIds.size(); ++i)
	{
		const Recipe* recipe = findBaseRecipeNoLock(recipeIds[i]);
		if (recipe == nullptr)
		{
			message = "菜谱不存在：" + recipeIds[i];
			return false;
		}
		if (!AccountStore::isRecipeOwned(*recipe, account))
		{
			message = "请先购买菜谱：" + recipe->m_title;
			return false;
		}
	}

	std::string realVideoId = videoId;
	if (realVideoId.empty() || m_videos.find(realVideoId) == m_videos.end())
	{
		video = VideoInfo();
		video.m_id = makeIdNoLock("video");
		video.m_ownerUserId = account.m_userId;
		video.m_mediaType = "video";
		video.m_filePath = g_config.m_videoStorageDirectoryName + "/" + video.m_id;
		video.m_createdAtSeconds = nowSecondsNoLock();
		m_videos[video.m_id] = video;
		realVideoId = video.m_id;
	}

	VideoInfo& storedVideo = m_videos[realVideoId];
	if (storedVideo.m_ownerUserId != account.m_userId)
	{
		message = "不能发布他人的视频";
		return false;
	}
	storedVideo.m_caption = caption;
	storedVideo.m_recipeIds = recipeIds;
	storedVideo.m_status = "published";
	video = storedVideo;
	saveNoLock();
	message = "发布成功";
	return true;
}

bool AccountStore::saveCustomRecipe(const std::string& userId,
                                    const Recipe& sourceRecipe,
                                    bool published,
                                    Recipe& recipe,
                                    std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	if (sourceRecipe.m_id.empty() || sourceRecipe.m_title.empty() || sourceRecipe.m_tasks.empty())
	{
		message = "菜谱ID、标题和步骤不能为空";
		return false;
	}
	std::map<std::string, Recipe>::const_iterator existingIt = m_customRecipes.find(sourceRecipe.m_id);
	if (existingIt != m_customRecipes.end() && existingIt->second.m_authorUserId != account.m_userId)
	{
		message = "菜谱ID已被使用";
		return false;
	}
	if (CookCatalog::findRecipe(sourceRecipe.m_id) != nullptr)
	{
		message = "不能覆盖系统菜谱";
		return false;
	}

	recipe = sourceRecipe;
	recipe.m_authorUserId = account.m_userId;
	recipe.m_author = account.m_nickname;
	recipe.m_systemRecipe = false;
	recipe.m_customRecipe = true;
	recipe.m_defaultOwned = false;
	recipe.m_status = published ? "published" : "draft";
	if (recipe.m_currentRevisionId.empty())
	{
		recipe.m_currentRevisionId = recipe.m_id + "_r" + CStringManager::toStringInt32(nowSecondsNoLock());
	}
	if (recipe.m_priceType.empty())
	{
		recipe.m_priceType = recipe.m_priceAmount > 0 || recipe.m_priceCoins > 0 ? "coin" : "free";
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
	for (size_t i = 0; i < recipe.m_tasks.size(); ++i)
	{
		if (recipe.m_tasks[i].m_shortId.empty())
		{
			recipe.m_tasks[i].m_shortId = CookHelper::taskShortId(recipe.m_tasks[i]);
		}
		recipe.m_tasks[i].m_recipeId = recipe.m_id;
		if (recipe.m_tasks[i].m_id.empty() || recipe.m_tasks[i].m_id.find(recipe.m_id + ".") != 0)
		{
			recipe.m_tasks[i].m_id = recipe.m_id + "." + recipe.m_tasks[i].m_shortId;
		}
	}

	m_customRecipes[recipe.m_id] = recipe;
	account.m_customRecipeIds.insert(recipe.m_id);
	account.m_ownedRecipeIds.insert(recipe.m_id);
	addMessageNoLock("system", account.m_userId, "recipe", published ? "菜谱发布成功" : "菜谱已保存草稿", "《" + recipe.m_title + "》已经保存。");
	saveNoLock();
	message = published ? "发布成功" : "保存成功";
	return true;
}

bool AccountStore::savePersonalization(const std::string& userId, const PersonalizationConfig& config, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	const Recipe* recipe = findBaseRecipeNoLock(config.m_recipeId);
	if (recipe == nullptr)
	{
		message = "菜谱不存在";
		return false;
	}
	if (!AccountStore::isRecipeOwned(*recipe, account))
	{
		message = "拥有菜谱后才能个性化";
		return false;
	}
	PersonalizationConfig storedConfig = config;
	storedConfig.m_userId = account.m_userId;
	storedConfig.m_recipeId = recipe->m_id;
	storedConfig.m_updatedAtSeconds = nowSecondsNoLock();
	m_personalizations[account.m_userId + "|" + recipe->m_id] = storedConfig;
	saveNoLock();
	message = "个性化设置已保存";
	return true;
}

bool AccountStore::syncPersonalizationDurations(const std::string& userId,
                                                const std::map<std::string, std::map<std::string, int32_t>>& recipeDurationOverrides,
                                                int32_t& updatedRecipeCount,
                                                int32_t& updatedTaskCount,
                                                std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	updatedRecipeCount = 0;
	updatedTaskCount = 0;
	if (recipeDurationOverrides.empty())
	{
		message = "没有可同步的步骤耗时";
		return false;
	}

	for (std::map<std::string, std::map<std::string, int32_t>>::const_iterator recipeIt = recipeDurationOverrides.begin();
		recipeIt != recipeDurationOverrides.end();
		++recipeIt)
	{
		const Recipe* recipe = findBaseRecipeNoLock(recipeIt->first);
		if (recipe == nullptr)
		{
			continue;
		}
		if (!AccountStore::isRecipeOwned(*recipe, account))
		{
			continue;
		}

		PersonalizationConfig config;
		std::map<std::string, PersonalizationConfig>::const_iterator existingIt = m_personalizations.find(account.m_userId + "|" + recipe->m_id);
		if (existingIt != m_personalizations.end())
		{
			config = existingIt->second;
		}
		else
		{
			config.m_userId = account.m_userId;
			config.m_recipeId = recipe->m_id;
		}

		bool recipeUpdated = false;
		for (std::map<std::string, int32_t>::const_iterator taskIt = recipeIt->second.begin();
			taskIt != recipeIt->second.end();
			++taskIt)
		{
			if (taskIt->first.empty() || taskIt->second < 0)
			{
				continue;
			}

			const CookTask* matchedTask = nullptr;
			for (size_t i = 0; i < recipe->m_tasks.size(); ++i)
			{
				if (CookHelper::taskIdMatches(recipe->m_tasks[i], taskIt->first))
				{
					matchedTask = &recipe->m_tasks[i];
					break;
				}
			}
			if (matchedTask == nullptr)
			{
				continue;
			}

			bool disabled = false;
			for (std::set<std::string>::const_iterator disabledIt = config.m_disabledTaskIds.begin();
				disabledIt != config.m_disabledTaskIds.end();
				++disabledIt)
			{
				if (CookHelper::taskIdMatches(*matchedTask, *disabledIt))
				{
					disabled = true;
					break;
				}
			}
			if (disabled)
			{
				continue;
			}

			config.m_durationOverrides[matchedTask->m_id] = taskIt->second;
			++updatedTaskCount;
			recipeUpdated = true;
		}

		if (recipeUpdated)
		{
			config.m_userId = account.m_userId;
			config.m_recipeId = recipe->m_id;
			config.m_updatedAtSeconds = nowSecondsNoLock();
			m_personalizations[account.m_userId + "|" + recipe->m_id] = config;
			++updatedRecipeCount;
		}
	}

	if (updatedTaskCount <= 0)
	{
		message = "没有匹配到可同步的步骤";
		return false;
	}
	saveNoLock();
	message = "已同步到个性化";
	return true;
}

bool AccountStore::resetPersonalization(const std::string& userId, const std::string& recipeId, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realRecipeId = normalizeRecipeIdNoLock(recipeId);
	m_personalizations.erase(account.m_userId + "|" + realRecipeId);
	saveNoLock();
	message = "已恢复默认菜谱";
	return true;
}

bool AccountStore::sharePersonalization(const std::string& userId, const std::string& recipeId, std::string& shareId, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realRecipeId = normalizeRecipeIdNoLock(recipeId);
	std::map<std::string, PersonalizationConfig>::const_iterator it = m_personalizations.find(account.m_userId + "|" + realRecipeId);
	if (it == m_personalizations.end())
	{
		message = "没有可分享的个性化设置";
		return false;
	}
	shareId = makeIdNoLock("share");
	m_sharedPersonalizations[shareId] = it->second;
	saveNoLock();
	message = "分享成功";
	return true;
}

bool AccountStore::applySharedPersonalization(const std::string& userId, const std::string& shareId, PersonalizationConfig& config, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::map<std::string, PersonalizationConfig>::const_iterator it = m_sharedPersonalizations.find(shareId);
	if (it == m_sharedPersonalizations.end())
	{
		message = "分享不存在";
		return false;
	}
	const Recipe* recipe = findBaseRecipeNoLock(it->second.m_recipeId);
	if (recipe == nullptr || !AccountStore::isRecipeOwned(*recipe, account))
	{
		message = "拥有菜谱后才能应用分享";
		return false;
	}
	config = it->second;
	config.m_userId = account.m_userId;
	config.m_updatedAtSeconds = nowSecondsNoLock();
	m_personalizations[account.m_userId + "|" + config.m_recipeId] = config;
	saveNoLock();
	message = "已应用分享设置";
	return true;
}

std::vector<Recipe> AccountStore::listRecipesForUser(const std::string& userId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::vector<Recipe> result;
	const std::vector<Recipe>& systemRecipes = CookCatalog::recipeCatalog();
	result.reserve(systemRecipes.size() + m_customRecipes.size());
	for (size_t i = 0; i < systemRecipes.size(); ++i)
	{
		Recipe recipe = systemRecipes[i];
		if (AccountStore::isRecipeOwned(recipe, account))
		{
			applyPersonalizationNoLock(account.m_userId, recipe);
		}
		result.push_back(recipe);
	}
	for (std::map<std::string, Recipe>::const_iterator it = m_customRecipes.begin(); it != m_customRecipes.end(); ++it)
	{
		Recipe recipe = it->second;
		if (AccountStore::isRecipeOwned(recipe, account))
		{
			applyPersonalizationNoLock(account.m_userId, recipe);
		}
		result.push_back(recipe);
	}
	return result;
}

bool AccountStore::getRecipeForUser(const std::string& userId, const std::string& recipeId, bool requireOwned, Recipe& recipe, std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	const Recipe* baseRecipe = findBaseRecipeNoLock(recipeId);
	if (baseRecipe == nullptr)
	{
		message = "菜谱不存在";
		return false;
	}
	if (requireOwned && !AccountStore::isRecipeOwned(*baseRecipe, account))
	{
		message = "请先购买菜谱：" + baseRecipe->m_title;
		return false;
	}
	recipe = *baseRecipe;
	if (AccountStore::isRecipeOwned(recipe, account))
	{
		applyPersonalizationNoLock(account.m_userId, recipe);
	}
	message = "ok";
	return true;
}

bool AccountStore::getPersonalizationDetail(const std::string& userId,
                                            const std::string& recipeId,
                                            Recipe& recipe,
                                            PersonalizationConfig& config,
                                            bool& hasConfig,
                                            std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	const Recipe* baseRecipe = findBaseRecipeNoLock(recipeId);
	config = PersonalizationConfig();
	hasConfig = false;
	if (baseRecipe == nullptr)
	{
		message = "菜谱不存在";
		return false;
	}
	if (!AccountStore::isRecipeOwned(*baseRecipe, account))
	{
		message = "拥有菜谱后才能个性化";
		return false;
	}
	recipe = *baseRecipe;
	std::map<std::string, PersonalizationConfig>::const_iterator it = m_personalizations.find(account.m_userId + "|" + baseRecipe->m_id);
	if (it != m_personalizations.end())
	{
		config = it->second;
		hasConfig = true;
	}
	else
	{
		config.m_userId = account.m_userId;
		config.m_recipeId = baseRecipe->m_id;
	}
	message = "ok";
	return true;
}

bool AccountStore::recordCookResult(const std::string& userId,
                                    const std::string& planId,
                                    const std::vector<std::string>& recipeIds,
                                    const std::string& imagePath,
                                    std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string text = "完成排程 " + planId + "，菜谱：" + CookHelper::joinVector(recipeIds, ',');
	if (!imagePath.empty())
	{
		text += "，成果：" + imagePath;
	}
	addMessageNoLock(account.m_userId, account.m_userId, "cook_result", "做菜成果已记录", text);
	saveNoLock();
	message = "成果已记录";
	return true;
}

std::vector<VideoInfo> AccountStore::listFeedVideosForUser(const std::string& userId,
                                                           int32_t count,
                                                           const std::set<std::string>& excludeVideoIds,
                                                           const std::string& mode,
                                                           const std::string& avoidVideoId,
                                                           bool& resetSeen)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	syncResourceVideosNoLock(false);
	resetSeen = false;
	int32_t safeCount = count <= 0 ? g_config.m_videoFeedDefaultCount : count;
	if (safeCount < 1)
	{
		safeCount = 1;
	}
	if (safeCount > 100)
	{
		safeCount = 100;
	}

	int32_t nowSeconds = nowSecondsNoLock();
	size_t watchedCountBeforePrune = account.m_watchedVideoAtSeconds.size();
	pruneExpiredVideoSeenNoLock(account, nowSeconds);
	bool needSave = watchedCountBeforePrune != account.m_watchedVideoAtSeconds.size();
	std::string realMode = CookHelper::lowerAscii(mode);
	if (realMode != "liked" && realMode != "favorite")
	{
		realMode.clear();
	}
	std::string realAvoidVideoId = CookHelper::normalizeVideoId(avoidVideoId);

	std::vector<VideoInfo> candidates;
	for (std::map<std::string, VideoInfo>::const_iterator it = m_videos.begin(); it != m_videos.end(); ++it)
	{
		if (it->second.m_status != "published" || it->second.m_mediaType != "video")
		{
			continue;
		}
		std::string filePath = resolveVideoFilePathNoLock(it->second.m_filePath);
		if (!CookHelper::isSupportedVideoFile(filePath) || !CSystem::DirOrFileExist(filePath))
		{
			continue;
		}
		std::string targetKey = targetKeyNoLock("video", it->second.m_id);
		if (realMode == "liked" && account.m_likedTargetIds.find(targetKey) == account.m_likedTargetIds.end())
		{
			continue;
		}
		if (realMode == "favorite" && account.m_favoriteTargetIds.find(targetKey) == account.m_favoriteTargetIds.end())
		{
			continue;
		}
		candidates.push_back(it->second);
	}

	std::vector<VideoInfo> eligible;
	int32_t unseenCount = 0;
	if (realMode.empty())
	{
		for (size_t i = 0; i < candidates.size(); ++i)
		{
			if (account.m_watchedVideoAtSeconds.find(candidates[i].m_id) == account.m_watchedVideoAtSeconds.end())
			{
				++unseenCount;
			}
		}
		for (size_t i = 0; i < candidates.size(); ++i)
		{
			bool seen = account.m_watchedVideoAtSeconds.find(candidates[i].m_id) != account.m_watchedVideoAtSeconds.end();
			bool excluded = excludeVideoIds.find(candidates[i].m_id) != excludeVideoIds.end();
			if (!seen && !excluded)
			{
				eligible.push_back(candidates[i]);
			}
		}
		if (eligible.empty() && !candidates.empty())
		{
			for (size_t i = 0; i < candidates.size(); ++i)
			{
				account.m_watchedVideoAtSeconds.erase(candidates[i].m_id);
			}
			resetSeen = true;
			needSave = true;
			for (size_t i = 0; i < candidates.size(); ++i)
			{
				if (excludeVideoIds.find(candidates[i].m_id) == excludeVideoIds.end())
				{
					eligible.push_back(candidates[i]);
				}
			}
			if (eligible.empty())
			{
				eligible = candidates;
			}
		}
	}
	else
	{
		eligible = candidates;
	}
	if (!realAvoidVideoId.empty() && eligible.size() > 1)
	{
		std::vector<VideoInfo> filtered;
		filtered.reserve(eligible.size());
		for (size_t i = 0; i < eligible.size(); ++i)
		{
			if (eligible[i].m_id != realAvoidVideoId)
			{
				filtered.push_back(eligible[i]);
			}
		}
		if (!filtered.empty())
		{
			eligible = filtered;
		}
	}

	std::vector<std::pair<std::string, VideoInfo>> scored;
	scored.reserve(eligible.size());
	std::string seed = CStringManager::toStringInt32(nowSeconds) + "|" + CStringManager::toStringInt32(static_cast<int32_t>(std::clock()));
	for (size_t i = 0; i < eligible.size(); ++i)
	{
		scored.push_back(std::make_pair(videoRecommendScoreNoLock(account.m_userId, eligible[i].m_id, seed), eligible[i]));
	}
	std::sort(scored.begin(), scored.end(),
		[](const std::pair<std::string, VideoInfo>& left, const std::pair<std::string, VideoInfo>& right)
		{
			return left.first == right.first ? left.second.m_id < right.second.m_id : left.first < right.first;
		});

	std::vector<VideoInfo> result;
	result.reserve(scored.size() < static_cast<size_t>(safeCount) ? scored.size() : static_cast<size_t>(safeCount));
	for (size_t i = 0; i < scored.size() && result.size() < static_cast<size_t>(safeCount); ++i)
	{
		result.push_back(scored[i].second);
	}
	if (needSave)
	{
		saveNoLock();
	}
	LOGINFO("AccountStore feed video select userId=%s mode=%s requestCount=%d candidateCount=%d unseenCount=%d eligibleCount=%d resultCount=%d excludeCount=%d avoidVideoId=%s resetSeen=%d expireDays=%d",
	        account.m_userId.c_str(),
	        realMode.c_str(),
	        safeCount,
	        static_cast<int32_t>(candidates.size()),
	        unseenCount,
	        static_cast<int32_t>(eligible.size()),
	        static_cast<int32_t>(result.size()),
	        static_cast<int32_t>(excludeVideoIds.size()),
	        realAvoidVideoId.c_str(),
	        resetSeen ? 1 : 0,
	        g_config.m_videoSeenExpireDays);
	return result;
}

bool AccountStore::markVideoWatched(const std::string& userId,
                                    const std::string& videoId,
                                    int32_t& watchedAtSeconds,
                                    std::string& message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	std::string realVideoId = CookHelper::normalizeVideoId(videoId);
	syncResourceVideosNoLock(false);
	std::map<std::string, VideoInfo>::const_iterator videoIt = m_videos.find(realVideoId);
	if (realVideoId.empty() || videoIt == m_videos.end() || videoIt->second.m_status != "published")
	{
		watchedAtSeconds = 0;
		message = "视频不存在";
		return false;
	}
	std::string filePath = resolveVideoFilePathNoLock(videoIt->second.m_filePath);
	if (!CSystem::DirOrFileExist(filePath))
	{
		watchedAtSeconds = 0;
		message = "视频文件不存在";
		return false;
	}
	watchedAtSeconds = nowSecondsNoLock();
	account.m_watchedVideoAtSeconds[videoIt->second.m_id] = watchedAtSeconds;
	saveNoLock();
	message = "ok";
	LOGINFO("AccountStore video watched userId=%s videoId=%s watchedAt=%d",
	        account.m_userId.c_str(),
	        videoIt->second.m_id.c_str(),
	        watchedAtSeconds);
	return true;
}

bool AccountStore::getVideoFile(const std::string& videoId, VideoInfo& video, std::string& filePath, std::string& contentType)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string realVideoId = CookHelper::normalizeVideoId(videoId);
	syncResourceVideosNoLock(false);
	std::map<std::string, VideoInfo>::const_iterator videoIt = m_videos.find(realVideoId);
	if (!realVideoId.empty() && videoIt == m_videos.end())
	{
		syncResourceVideosNoLock(true);
		videoIt = m_videos.find(realVideoId);
	}
	if (realVideoId.empty() || videoIt == m_videos.end() || videoIt->second.m_status != "published")
	{
		return false;
	}
	std::string resolvedPath = resolveVideoFilePathNoLock(videoIt->second.m_filePath);
	if (!CookHelper::isSupportedVideoFile(resolvedPath) || !CSystem::DirOrFileExist(resolvedPath))
	{
		return false;
	}
	video = videoIt->second;
	filePath = resolvedPath;
	contentType = CookHelper::videoContentType(filePath);
	return true;
}

int32_t AccountStore::countTargetLikes(const std::string& targetType, const std::string& targetId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string key = targetKeyNoLock(targetType, targetId);
	int32_t count = 0;
	for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
	{
		if (it->second.m_likedTargetIds.find(key) != it->second.m_likedTargetIds.end())
		{
			++count;
		}
	}
	return count;
}

int32_t AccountStore::countTargetComments(const std::string& targetType, const std::string& targetId)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	loadIfNeededNoLock();
	std::string realType = targetType.empty() ? "recipe" : targetType;
	std::string realTargetId = realType == "recipe" ? normalizeRecipeIdNoLock(targetId) : targetId;
	int32_t count = 0;
	for (size_t i = 0; i < m_comments.size(); ++i)
	{
		if (m_comments[i].m_targetType == realType && m_comments[i].m_targetId == realTargetId)
		{
			++count;
		}
	}
	return count;
}
