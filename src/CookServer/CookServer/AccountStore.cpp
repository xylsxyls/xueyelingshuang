#include "AccountStore.h"
#include "Config.h"
#include "CookCatalog.h"
#include "CookServerHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <fstream>

bool AccountStore::isRecipeOwned(const Recipe& recipe, const UserAccount& account)
{
	return recipe.m_priceCoins <= 0 || account.m_purchasedRecipeIds.find(recipe.m_id) != account.m_purchasedRecipeIds.end();
}

AccountStore::AccountStore() :
m_loaded(false)
{

}

std::string AccountStore::normalizeUserId(const std::string& userId) const
{
	return userId.empty() ? "demo_user" : userId;
}

std::string AccountStore::stateFilePath() const
{
	return CSystem::GetCurrentExePath() + g_config.m_accountStateFileName;
}

void AccountStore::loadIfNeededNoLock()
{
	if (m_loaded)
	{
		return;
	}
	m_loaded = true;

	std::string path = stateFilePath();
	std::ifstream input(path.c_str());
	if (!input.is_open())
	{
		LOGINFO("AccountStore state file not found, start with empty state path=%s", path.c_str());
		return;
	}

	std::string line;
	while (std::getline(input, line))
	{
		std::vector<std::string> parts = CookServerHelper::split(line, '\t');
		if (parts.size() < 4 || parts[0] != "USER")
		{
			continue;
		}

		UserAccount account;
		account.m_userId = parts[1];
		account.m_coins = static_cast<int32_t>(CStringManager::atoi64(parts[2].c_str()));

		std::vector<std::string> purchased = CookServerHelper::split(parts[3], ',');
		account.m_purchasedRecipeIds.insert(purchased.begin(), purchased.end());

		if (parts.size() >= 5)
		{
			std::vector<std::string> favorites = CookServerHelper::split(parts[4], ',');
			account.m_favoriteRecipeIds.insert(favorites.begin(), favorites.end());
		}

		if (!account.m_userId.empty())
		{
			m_accounts[account.m_userId] = account;
		}
	}
	LOGINFO("AccountStore state loaded path=%s accountCount=%d", path.c_str(), static_cast<int32_t>(m_accounts.size()));
}

void AccountStore::saveNoLock()
{
	std::string path = stateFilePath();
	std::ofstream output(path.c_str(), std::ios::trunc);
	if (!output.is_open())
	{
		LOGERROR("AccountStore save failed, open file failed path=%s", path.c_str());
		return;
	}

	for (std::map<std::string, UserAccount>::const_iterator it = m_accounts.begin(); it != m_accounts.end(); ++it)
	{
		const UserAccount& account = it->second;
		output << "USER"
			<< "\t" << account.m_userId
			<< "\t" << account.m_coins
			<< "\t" << CookServerHelper::joinSet(account.m_purchasedRecipeIds, ',')
			<< "\t" << CookServerHelper::joinSet(account.m_favoriteRecipeIds, ',')
			<< "\n";
	}
	output.flush();
	if (!output.good())
	{
		LOGERROR("AccountStore save failed, write file failed path=%s accountCount=%d",
		         path.c_str(),
		         static_cast<int32_t>(m_accounts.size()));
		return;
	}
	LOGDEBUG("AccountStore state saved path=%s accountCount=%d", path.c_str(), static_cast<int32_t>(m_accounts.size()));
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
		account.m_coins = g_config.m_accountDefaultCoins;
		m_accounts[id] = account;
		saveNoLock();
		LOGINFO("AccountStore create account userId=%s defaultCoins=%d", id.c_str(), g_config.m_accountDefaultCoins);
	}
	return m_accounts[id];
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
		LOGWARNING("AccountStore recharge rejected userId=%s coins=%d maxCoins=%d",
		           account.m_userId.c_str(),
		           coins,
		           g_config.m_accountMaxRechargeCoins);
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
	LOGINFO("AccountStore recharge success userId=%s coins=%d balance=%d",
	        account.m_userId.c_str(),
	        coins,
	        account.m_coins);
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

UserAccount AccountStore::purchaseRecipe(const std::string& userId, const std::string& recipeId, bool* ok, std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	const Recipe* recipe = CookCatalog::findRecipe(recipeId);
	if (recipe == nullptr)
	{
		LOGWARNING("AccountStore purchase rejected, recipe not found userId=%s recipeId=%s",
		           account.m_userId.c_str(),
		           recipeId.c_str());
		if (ok)
		{
			*ok = false;
		}
		if (message)
		{
			*message = "菜谱不存在";
		}
		return account;
	}
	if (AccountStore::isRecipeOwned(*recipe, account))
	{
		LOGINFO("AccountStore purchase ignored, recipe already owned userId=%s recipeId=%s",
		        account.m_userId.c_str(),
		        recipeId.c_str());
		if (ok)
		{
			*ok = true;
		}
		if (message)
		{
			*message = "已经拥有该菜谱";
		}
		return account;
	}
	if (account.m_coins < recipe->m_priceCoins)
	{
		LOGWARNING("AccountStore purchase rejected, coins not enough userId=%s recipeId=%s balance=%d price=%d",
		           account.m_userId.c_str(),
		           recipeId.c_str(),
		           account.m_coins,
		           recipe->m_priceCoins);
		if (ok)
		{
			*ok = false;
		}
		if (message)
		{
			*message = "金币不足，请先充值";
		}
		return account;
	}
	account.m_coins -= recipe->m_priceCoins;
	account.m_purchasedRecipeIds.insert(recipe->m_id);
	saveNoLock();
	LOGINFO("AccountStore purchase success userId=%s recipeId=%s price=%d balance=%d",
	        account.m_userId.c_str(),
	        recipeId.c_str(),
	        recipe->m_priceCoins,
	        account.m_coins);
	if (ok)
	{
		*ok = true;
	}
	if (message)
	{
		*message = "购买成功，已加入菜单";
	}
	return account;
}

UserAccount AccountStore::toggleFavorite(const std::string& userId, const std::string& recipeId, bool* ok, bool* favorite, std::string* message)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	UserAccount& account = ensureNoLock(userId);
	const Recipe* recipe = CookCatalog::findRecipe(recipeId);
	if (recipe == nullptr)
	{
		LOGWARNING("AccountStore favorite rejected, recipe not found userId=%s recipeId=%s",
		           account.m_userId.c_str(),
		           recipeId.c_str());
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

	bool isFavorite = false;
	std::set<std::string>::iterator it = account.m_favoriteRecipeIds.find(recipeId);
	if (it == account.m_favoriteRecipeIds.end())
	{
		account.m_favoriteRecipeIds.insert(recipeId);
		isFavorite = true;
		if (favorite)
		{
			*favorite = true;
		}
	}
	else
	{
		account.m_favoriteRecipeIds.erase(it);
		isFavorite = false;
		if (favorite)
		{
			*favorite = false;
		}
	}

	saveNoLock();
	LOGINFO("AccountStore favorite toggled userId=%s recipeId=%s favorite=%d",
	        account.m_userId.c_str(),
	        recipeId.c_str(),
	        isFavorite ? 1 : 0);
	if (ok)
	{
		*ok = true;
	}
	if (message)
	{
		*message = "收藏状态已更新";
	}
	return account;
}