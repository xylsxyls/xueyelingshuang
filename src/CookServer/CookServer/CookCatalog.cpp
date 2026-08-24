#include "CookCatalog.h"
#include "Config.h"
#include "CookRecipeConfig.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <set>

std::vector<Recipe> CookCatalog::m_recipeCatalog;
bool CookCatalog::m_isInit = false;

bool CookCatalog::init()
{
	if (m_isInit)
	{
		CookCatalog::uninit();
	}
	m_recipeCatalog = CookCatalog::loadRecipeCatalog();
	m_isInit = true;
	return !m_recipeCatalog.empty();
}

void CookCatalog::uninit()
{
	if (!m_isInit)
	{
		return;
	}
	m_recipeCatalog.clear();
	m_isInit = false;
}

std::string CookCatalog::recipeResourceDirectoryPath()
{
	std::string path = CSystem::GetCurrentExePath() + g_config.m_recipeResourceDirectoryName;
#ifdef _WIN32
	const char separator = '\\';
#else
	const char separator = '/';
#endif
	if (!path.empty() && path[path.size() - 1] != '\\' && path[path.size() - 1] != '/')
	{
		path.push_back(separator);
	}
	return path;
}

bool CookCatalog::loadRecipeDirectory(const std::string& directoryPath, std::vector<Recipe>& recipes, std::string* errorMessage)
{
	if (directoryPath.empty())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "recipe resource directory path is empty";
		}
		return false;
	}
	if (!CSystem::DirOrFileExist(directoryPath))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "recipe resource directory not found: " + directoryPath;
		}
		return false;
	}

	std::vector<std::string> files = CSystem::findFilePath(directoryPath, 2, "json");
	std::sort(files.begin(), files.end());
	if (files.empty())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "recipe resource directory has no json file: " + directoryPath;
		}
		return false;
	}

	std::set<std::string> recipeIds;
	std::vector<Recipe> loadedRecipes;
	loadedRecipes.reserve(files.size());
	for (size_t i = 0; i < files.size(); ++i)
	{
		if (CSystem::GetName(files[i], 4) != directoryPath)
		{
			continue;
		}

		Recipe recipe;
		std::string parseError;
		if (!CookRecipeConfig::loadRecipeFromFile(files[i], recipe, &parseError))
		{
			LOGWARNING("CookCatalog skip recipe file path=%s error=%s", files[i].c_str(), parseError.c_str());
			continue;
		}

		std::string fileRecipeId = CSystem::GetName(files[i], 3);
		if (fileRecipeId != recipe.m_id)
		{
			LOGWARNING("CookCatalog skip recipe file, file name and recipe id not match path=%s fileRecipeId=%s recipeId=%s",
			           files[i].c_str(),
			           fileRecipeId.c_str(),
			           recipe.m_id.c_str());
			continue;
		}
		if (recipeIds.find(recipe.m_id) != recipeIds.end())
		{
			LOGWARNING("CookCatalog skip duplicated recipe id path=%s recipeId=%s", files[i].c_str(), recipe.m_id.c_str());
			continue;
		}
		recipeIds.insert(recipe.m_id);
		loadedRecipes.push_back(recipe);
	}

	if (loadedRecipes.empty())
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "recipe resource directory has no valid recipe: " + directoryPath;
		}
		return false;
	}
	recipes.swap(loadedRecipes);
	return true;
}

void CookCatalog::addTask(
	Recipe& recipe,
	const std::string& id,
	const std::string& title,
	const std::string& detail,
	const std::string& resource,
	int32_t durationSeconds,
	bool active,
	bool canPause,
	bool continuesDuringPause,
	bool canLeaveKitchen,
	const std::string& safetyLevel,
	const std::vector<std::string>& dependencies)
{
	CookTask task;
	task.m_id = recipe.m_id + "." + id;
	task.m_shortId = id;
	task.m_recipeId = recipe.m_id;
	task.m_title = title;
	task.m_detail = detail;
	task.m_resource = resource;
	task.m_voiceText = title + "。" + detail;
	task.m_safetyLevel = safetyLevel;
	task.m_durationSeconds = durationSeconds;
	task.m_active = active;
	task.m_manualSkippable = active;
	task.m_canPause = canPause;
	task.m_continuesDuringPause = continuesDuringPause;
	task.m_backgroundWaitMode = "";
	if (!active)
	{
		task.m_backgroundWaitMode = canLeaveKitchen && safetyLevel != "attention" && safetyLevel != "danger" ? "free" : "watch";
	}
	task.m_canLeaveKitchen = !active && task.m_backgroundWaitMode == "free";
	for (size_t i = 0; i < dependencies.size(); ++i)
	{
		task.m_dependencies.push_back(recipe.m_id + "." + dependencies[i]);
	}
	recipe.m_tasks.push_back(task);
}

std::vector<Recipe> CookCatalog::buildBuiltinRecipeCatalog()
{
	std::vector<Recipe> recipes;

	Recipe ribs;
	ribs.m_id = "cook_000001";
	ribs.m_title = "糖醋排骨";
	ribs.m_category = "荤菜";
	ribs.m_subtitle = "新手版，先浸泡去血水，再小火煮透，最后大火收汁。";
	ribs.m_difficulty = "新手可做";
	ribs.m_coverColor = "linear-gradient(135deg,#b83b2b,#f0a35e)";
	ribs.m_author = "系统菜谱";
	ribs.m_tags = {"下饭", "可并行", "守火"};
	ribs.m_ingredients = {{"排骨", "500g"}, {"冰糖", "20g"}, {"生抽", "2勺"}, {"香醋", "2勺"}, {"葱", "1根"}};
	ribs.m_tools = {"炒锅", "砧板", "菜刀", "大碗"};
	ribs.m_priceCoins = 0;
	ribs.m_defaultOwned = true;
	ribs.m_systemRecipe = true;
	CookCatalog::addTask(ribs, "wash_meat", "冲洗排骨", "冷水冲洗排骨，抓洗到表面没有明显血水。", "sink", 120, true, true, false, false, "normal", {});
	CookCatalog::addTask(ribs, "soak_meat", "浸泡排骨", "排骨放入冷水中浸泡，中途不用动手，结束后倒掉血水。", "", 600, false, false, true, true, "normal", {"wash_meat"});
	CookCatalog::addTask(ribs, "cut_scallion", "洗葱切葱花", "葱洗净，葱白切段，葱绿切葱花，最后收汁时会用到。", "board", 180, true, true, false, false, "normal", {"wash_meat"});
	CookCatalog::addTask(ribs, "drain_meat", "倒水沥干", "倒掉浸泡水，用厨房纸或漏勺把排骨沥到不滴水。", "sink", 60, true, true, false, false, "normal", {"soak_meat"});
	CookCatalog::addTask(ribs, "stir_fry", "炒排骨上色", "锅热后倒少量油，排骨下锅，中火翻炒到表面微黄。", "stove", 300, true, false, false, false, "attention", {"drain_meat"});
	CookCatalog::addTask(ribs, "simmer", "加调料小火煮", "加入糖、生抽、醋和热水没过排骨，转小火盖盖煮。", "stove", 1500, false, false, true, true, "normal", {"stir_fry"});
	CookCatalog::addTask(ribs, "high_heat", "开大火收汁", "打开锅盖，转大火，让汤汁明显翻滚。", "stove", 300, false, false, true, false, "attention", {"simmer"});
	CookCatalog::addTask(ribs, "reduce_stir", "翻炒收汁", "持续翻炒，让酱汁均匀裹住排骨，撒葱花后关火。", "stove", 300, true, false, false, false, "danger", {"high_heat", "cut_scallion"});
	recipes.push_back(ribs);

	Recipe rice;
	rice.m_id = "cook_000002";
	rice.m_title = "电饭锅米饭";
	rice.m_category = "主食";
	rice.m_subtitle = "淘米后交给电饭锅，适合插入肉类浸泡或炖煮窗口。";
	rice.m_difficulty = "零基础";
	rice.m_coverColor = "linear-gradient(135deg,#e4e0ca,#89a36a)";
	rice.m_author = "系统菜谱";
	rice.m_tags = {"主食", "自由时间", "电饭锅"};
	rice.m_ingredients = {{"大米", "1杯"}, {"清水", "约1.1杯"}};
	rice.m_tools = {"电饭锅", "量杯"};
	rice.m_priceCoins = 0;
	rice.m_defaultOwned = true;
	rice.m_systemRecipe = true;
	CookCatalog::addTask(rice, "wash_rice", "淘米加水", "米淘洗到水略清，按刻度加水，把内胆放进电饭锅。", "sink", 300, true, true, false, false, "normal", {});
	CookCatalog::addTask(rice, "cook_rice", "启动煮饭", "按下煮饭键，等电饭锅完成。", "rice_cooker", 2100, false, false, true, true, "normal", {"wash_rice"});
	CookCatalog::addTask(rice, "fluff_rice", "翻松米饭", "饭熟后开盖，用饭勺把米饭翻松，避免结块。", "rice_cooker", 60, true, true, false, false, "normal", {"cook_rice"});
	recipes.push_back(rice);

	Recipe eggs;
	eggs.m_id = "cook_000003";
	eggs.m_title = "番茄炒蛋";
	eggs.m_category = "小荤";
	eggs.m_subtitle = "先炒蛋再炒番茄，盐和糖后放，味道更稳。";
	eggs.m_difficulty = "新手可做";
	eggs.m_coverColor = "linear-gradient(135deg,#f35b3f,#ffd56b)";
	eggs.m_author = "系统菜谱";
	eggs.m_tags = {"快手", "下饭", "少油"};
	eggs.m_ingredients = {{"番茄", "2个"}, {"鸡蛋", "2个"}, {"盐", "少许"}, {"糖", "半勺"}};
	eggs.m_tools = {"炒锅", "碗", "砧板"};
	eggs.m_priceCoins = 0;
	eggs.m_defaultOwned = true;
	eggs.m_systemRecipe = true;
	CookCatalog::addTask(eggs, "wash_tomato", "洗番茄", "番茄冲洗干净，去掉蒂部。", "sink", 120, true, true, false, false, "normal", {});
	CookCatalog::addTask(eggs, "cut_tomato", "切番茄", "番茄切滚刀块，汁水尽量留在盘子里。", "board", 120, true, true, false, false, "normal", {"wash_tomato"});
	CookCatalog::addTask(eggs, "beat_eggs", "打散鸡蛋", "鸡蛋打入碗里，加一点盐，搅到没有大块蛋清。", "board", 180, true, true, false, false, "normal", {});
	CookCatalog::addTask(eggs, "scramble_eggs", "先炒鸡蛋", "热锅热油，倒入蛋液，凝固后推成大块盛出。", "stove", 180, true, false, false, false, "attention", {"beat_eggs"});
	CookCatalog::addTask(eggs, "cook_tomato", "炒番茄回蛋", "番茄下锅炒软出汁，倒回鸡蛋，加盐和少量糖。", "stove", 360, true, false, false, false, "attention", {"cut_tomato", "scramble_eggs"});
	recipes.push_back(eggs);

	Recipe greens;
	greens.m_id = "cook_000004";
	greens.m_title = "蒜蓉青菜";
	greens.m_category = "蔬菜";
	greens.m_subtitle = "大火快炒，盐最后放，保留青菜脆感。";
	greens.m_difficulty = "零基础";
	greens.m_coverColor = "linear-gradient(135deg,#27784f,#b7d968)";
	greens.m_author = "系统菜谱";
	greens.m_tags = {"快手", "蔬菜", "最后放盐"};
	greens.m_ingredients = {{"青菜", "300g"}, {"蒜", "3瓣"}, {"盐", "少许"}};
	greens.m_tools = {"炒锅", "菜篮", "砧板"};
	greens.m_priceCoins = 0;
	greens.m_defaultOwned = true;
	greens.m_systemRecipe = true;
	CookCatalog::addTask(greens, "wash_greens", "洗青菜", "青菜掰开冲洗，根部泥沙要冲掉，沥到不大量滴水。", "sink", 180, true, true, false, false, "normal", {});
	CookCatalog::addTask(greens, "cut_garlic", "切蒜末", "蒜拍扁去皮，切成蒜末。", "board", 120, true, true, false, false, "normal", {"wash_greens"});
	CookCatalog::addTask(greens, "stir_greens", "大火炒青菜", "油热后下蒜末，出香味后下青菜，大火快速翻炒。", "stove", 300, true, false, false, false, "danger", {"cut_garlic"});
	CookCatalog::addTask(greens, "season_greens", "最后放盐", "青菜变软后再放盐，翻匀马上出锅。", "stove", 60, true, false, false, false, "attention", {"stir_greens"});
	recipes.push_back(greens);

	Recipe soup;
	soup.m_id = "cook_000005";
	soup.m_title = "豆腐紫菜汤";
	soup.m_category = "汤饮";
	soup.m_subtitle = "等菜时顺手煮一锅，最后下紫菜和葱花。";
	soup.m_difficulty = "零基础";
	soup.m_coverColor = "linear-gradient(135deg,#6ba4b8,#f1dfb2)";
	soup.m_author = "系统菜谱";
	soup.m_tags = {"汤", "清淡", "快手"};
	soup.m_ingredients = {{"嫩豆腐", "半盒"}, {"紫菜", "少许"}, {"盐", "少许"}, {"葱花", "少许"}};
	soup.m_tools = {"小锅", "汤勺"};
	soup.m_priceCoins = 0;
	soup.m_defaultOwned = true;
	soup.m_systemRecipe = true;
	CookCatalog::addTask(soup, "cut_tofu", "切豆腐", "豆腐切成小块，动作轻一点，避免碎成渣。", "board", 120, true, true, false, false, "normal", {});
	CookCatalog::addTask(soup, "boil_water", "烧水下豆腐", "小锅加水烧开，下豆腐煮到再次沸腾。", "stove", 480, false, false, true, false, "attention", {"cut_tofu"});
	CookCatalog::addTask(soup, "season_soup", "调味下紫菜", "放盐，关火前下紫菜和葱花。", "stove", 120, true, false, false, false, "attention", {"boil_water"});
	recipes.push_back(soup);

	Recipe beef;
	beef.m_id = "cook_000006";
	beef.m_title = "黑椒牛柳";
	beef.m_category = "荤菜";
	beef.m_subtitle = "付费示例菜谱，先腌后快炒，适合练习肉类不过老。";
	beef.m_difficulty = "进阶新手";
	beef.m_coverColor = "linear-gradient(135deg,#3a2a24,#b49768)";
	beef.m_author = "示例创作者";
	beef.m_tags = {"付费", "快炒", "高蛋白"};
	beef.m_ingredients = {{"牛里脊", "250g"}, {"青椒", "1个"}, {"洋葱", "半个"}, {"黑胡椒酱", "1勺"}};
	beef.m_tools = {"炒锅", "砧板", "碗"};
	beef.m_priceCoins = 18;
	beef.m_defaultOwned = false;
	beef.m_systemRecipe = false;
	CookCatalog::addTask(beef, "slice_beef", "切牛肉", "牛肉逆纹切条，厚薄尽量一致。", "board", 240, true, true, false, false, "normal", {});
	CookCatalog::addTask(beef, "marinate_beef", "腌牛肉", "加生抽、淀粉和一点油抓匀，静置入味。", "board", 600, false, false, true, true, "normal", {"slice_beef"});
	CookCatalog::addTask(beef, "cut_veg", "切青椒洋葱", "青椒去籽切条，洋葱切粗丝。", "board", 300, true, true, false, false, "normal", {"slice_beef"});
	CookCatalog::addTask(beef, "fry_beef", "滑炒牛肉", "热锅热油，牛肉快速炒到变色立刻盛出。", "stove", 180, true, false, false, false, "danger", {"marinate_beef"});
	CookCatalog::addTask(beef, "fry_veg", "炒配菜回牛肉", "青椒洋葱炒香，倒回牛肉，加黑椒酱快速翻匀。", "stove", 300, true, false, false, false, "attention", {"cut_veg", "fry_beef"});
	recipes.push_back(beef);

	Recipe chicken;
	chicken.m_id = "cook_000007";
	chicken.m_title = "照烧鸡腿";
	chicken.m_category = "小荤";
	chicken.m_subtitle = "付费示例菜谱，煎鸡腿和收汁分开提醒，适合工作日晚餐。";
	chicken.m_difficulty = "新手可做";
	chicken.m_coverColor = "linear-gradient(135deg,#8d4b35,#f0c16c)";
	chicken.m_author = "示例创作者";
	chicken.m_tags = {"付费", "便当", "下饭"};
	chicken.m_ingredients = {{"鸡腿肉", "1块"}, {"生抽", "2勺"}, {"蜂蜜", "1勺"}, {"料酒", "1勺"}};
	chicken.m_tools = {"平底锅", "碗", "砧板"};
	chicken.m_priceCoins = 0;
	chicken.m_priceType = "yuanbao";
	chicken.m_priceAmount = 22;
	chicken.m_defaultOwned = false;
	chicken.m_systemRecipe = false;
	CookCatalog::addTask(chicken, "trim_chicken", "处理鸡腿肉", "鸡腿肉擦干，厚的地方划两刀，方便受热均匀。", "board", 180, true, true, false, false, "normal", {});
	CookCatalog::addTask(chicken, "mix_sauce", "调照烧汁", "生抽、料酒、蜂蜜和少量清水调匀。", "board", 120, true, true, false, false, "normal", {"trim_chicken"});
	CookCatalog::addTask(chicken, "sear_chicken", "煎鸡腿", "鸡皮朝下中火煎到金黄，再翻面煎。", "stove", 600, true, false, false, false, "attention", {"mix_sauce"});
	CookCatalog::addTask(chicken, "reduce_sauce", "倒汁收浓", "倒入照烧汁，小火煮到酱汁变稠，中途翻面。", "stove", 420, false, false, true, false, "attention", {"sear_chicken"});
	CookCatalog::addTask(chicken, "slice_plate", "切块装盘", "鸡腿肉稍微放一分钟再切，淋上锅里剩下的酱汁。", "board", 180, true, true, false, false, "normal", {"reduce_sauce"});
	recipes.push_back(chicken);

	for (size_t i = 0; i < recipes.size(); ++i)
	{
		if (recipes[i].m_priceType.empty())
		{
			recipes[i].m_priceType = recipes[i].m_priceCoins > 0 ? "coin" : "free";
		}
		if (recipes[i].m_priceAmount <= 0)
		{
			recipes[i].m_priceAmount = recipes[i].m_priceCoins;
		}
		if (recipes[i].m_priceType == "free")
		{
			recipes[i].m_priceAmount = 0;
			recipes[i].m_priceCoins = 0;
		}
		if (recipes[i].m_status.empty())
		{
			recipes[i].m_status = "published";
		}
		if (recipes[i].m_currentRevisionId.empty())
		{
			recipes[i].m_currentRevisionId = recipes[i].m_id + "_r1";
		}
		if (recipes[i].m_authorUserId.empty())
		{
			recipes[i].m_authorUserId = recipes[i].m_systemRecipe ? "system" : "creator_demo";
		}
		recipes[i].m_customRecipe = false;
	}

	return recipes;
}

std::vector<Recipe> CookCatalog::loadRecipeCatalog()
{
	std::vector<Recipe> recipes;
	std::string errorMessage;
	std::string directoryPath = CookCatalog::recipeResourceDirectoryPath();
	if (CookCatalog::loadRecipeDirectory(directoryPath, recipes, &errorMessage))
	{
		LOGINFO("CookCatalog loaded recipe resource directory path=%s schemaVersion=%s recipeCount=%d",
		        directoryPath.c_str(),
		        CookRecipeConfig::supportedSchemaVersion().c_str(),
		        static_cast<int32_t>(recipes.size()));
		return recipes;
	}

	LOGWARNING("CookCatalog load recipe resource directory failed path=%s error=%s, use builtin catalog",
	           directoryPath.c_str(),
	           errorMessage.c_str());
	recipes = CookCatalog::buildBuiltinRecipeCatalog();
	LOGINFO("CookCatalog builtin recipe catalog ready recipeCount=%d", static_cast<int32_t>(recipes.size()));
	return recipes;
}

const std::vector<Recipe>& CookCatalog::recipeCatalog()
{
	return m_recipeCatalog;
}

const Recipe* CookCatalog::findRecipe(const std::string& id)
{
	const std::vector<Recipe>& recipes = CookCatalog::recipeCatalog();
	for (size_t i = 0; i < recipes.size(); ++i)
	{
		if (recipes[i].m_id == id)
		{
			return &recipes[i];
		}
	}
	return nullptr;
}