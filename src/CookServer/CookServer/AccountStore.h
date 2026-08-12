#pragma once
#include "CookModels.h"
#include <map>
#include <mutex>

/** 简易账号仓库，负责本地保存金币、已购买菜谱和收藏菜谱状态
*/
class AccountStore
{
public:
	/** 构造函数
	*/
	AccountStore();

	/** 判断账号是否已经拥有指定菜谱
	@param [in] recipe 待判断的菜谱
	@param [in] account 待判断的账号
	@return 返回true表示免费菜谱或账号已购买该菜谱
	*/
	static bool isRecipeOwned(const Recipe& recipe, const UserAccount& account);

	/** 获取账号信息，不存在时会创建默认账号
	@param [in] userId 用户ID，空字符串会被归一化为demo_user
	@return 返回账号快照
	*/
	UserAccount getAccount(const std::string& userId);

	/** 给账号充值金币
	@param [in] userId 用户ID
	@param [in] coins 充值金币数，必须为正数且不能过大
	@param [out] ok 返回充值是否成功，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount recharge(const std::string& userId, int32_t coins, bool* ok, std::string* message);

	/** 购买菜谱，金币不足或菜谱不存在时不会扣费
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] ok 返回购买是否成功，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount purchaseRecipe(const std::string& userId, const std::string& recipeId, bool* ok, std::string* message);

	/** 切换菜谱收藏状态
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] ok 返回操作是否成功，可为空
	@param [out] favorite 返回操作后是否已收藏，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount toggleFavorite(const std::string& userId, const std::string& recipeId, bool* ok, bool* favorite, std::string* message);

private:
	/** 归一化用户ID
	@param [in] userId 用户ID
	@return 返回可用于持久化的用户ID
	*/
	std::string normalizeUserId(const std::string& userId) const;

	/** 获取账号状态文件完整路径
	@return 返回exe同级目录下的状态文件路径
	*/
	std::string stateFilePath() const;

	/** 在未加载时从本地状态文件读取账号数据，调用方必须已持有锁
	*/
	void loadIfNeededNoLock();

	/** 将内存中的账号数据写回本地状态文件，调用方必须已持有锁
	*/
	void saveNoLock();

	/** 确保账号存在，调用方必须已持有锁
	@param [in] userId 用户ID
	@return 返回可修改的账号引用
	*/
	UserAccount& ensureNoLock(const std::string& userId);

private:
	// 保护账号内存表和加载状态
	std::mutex m_mutex;
	// 已加载的账号表，key为用户ID
	std::map<std::string, UserAccount> m_accounts;
	// 是否已经尝试加载过本地状态文件
	bool m_loaded;
};