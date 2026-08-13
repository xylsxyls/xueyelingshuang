#pragma once
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

/** CookServer做菜业务模型集合
这些结构体只描述菜谱、排程和账号状态，不负责数据库、网络或HTTP协议细节。
*/

/** 菜谱中单个食材的基础信息，用于制作前准备清单和菜谱详情展示
*/
struct Ingredient
{
	// 食材名称，例如排骨、葱、鸡腿肉
	std::string m_name;
	// 食材用量，保留作者填写的展示文本
	std::string m_amount;
};

/** 一个可被排程系统安排到时间线上的做菜步骤
*/
struct CookTask
{
	/** 构造函数
	*/
	CookTask();

	// 步骤ID，在单个菜谱内由菜谱ID和步骤短ID组合生成
	std::string m_id;
	// 步骤所属菜谱ID
	std::string m_recipeId;
	// 步骤标题，适合在时间线和语音提醒中短句展示
	std::string m_title;
	// 步骤详细说明，面向零基础用户描述做到什么程度
	std::string m_detail;
	// 步骤占用的厨房资源，例如灶台、水槽、砧板；为空表示不占独占资源
	std::string m_resource;
	// 语音播报文本，由步骤标题和细节组合而来
	std::string m_voiceText;
	// 安全提示级别，用于前端决定提醒强度
	std::string m_safetyLevel;
	// 依赖步骤ID列表，当前步骤必须等这些步骤完成后才能开始
	std::vector<std::string> m_dependencies;
	// 步骤持续秒数，包含等待和操作时间
	int32_t m_durationSeconds;
	// 是否需要用户持续动手操作
	bool m_active;
	// 用户暂停流程时，该步骤是否允许一起暂停
	bool m_canPause;
	// 用户暂停流程时，该步骤是否仍会自然继续，例如浸泡、焖煮
	bool m_continuesDuringPause;
	// 该步骤进行期间，用户是否可以离开厨房做其他事
	bool m_canLeaveKitchen;
};

/** 菜谱完整数据，包含展示信息、售卖信息、食材工具和可排程步骤
*/
struct Recipe
{
	/** 构造函数
	*/
	Recipe();

	// 菜谱ID，作为购买、收藏、排程时的稳定标识
	std::string m_id;
	// 菜谱标题
	std::string m_title;
	// 菜谱分类，用于菜单左侧分类和搜索筛选
	std::string m_category;
	// 菜谱短描述，用于信息流卡片和详情页摘要
	std::string m_subtitle;
	// 难度标签，给用户判断是否适合当前能力
	std::string m_difficulty;
	// 示例封面颜色，简易版前端用它模拟封面视觉
	std::string m_coverColor;
	// 作者名称
	std::string m_author;
	// 菜谱标签，用于展示和后续推荐
	std::vector<std::string> m_tags;
	// 制作前需要准备的食材列表
	std::vector<Ingredient> m_ingredients;
	// 制作时需要的工具列表
	std::vector<std::string> m_tools;
	// 可被排程算法使用的步骤列表
	std::vector<CookTask> m_tasks;
	// 购买所需金币，0表示免费
	int32_t m_priceCoins;
	// 新账号是否默认拥有该菜谱，真正的账号拥有关系仍以UserAccount中的菜谱ID集合为准
	bool m_defaultOwned;
	// 是否为系统内置菜谱
	bool m_systemRecipe;
};

/** 已经计算出开始和结束时间的做菜步骤
*/
struct ScheduledTask : public CookTask
{
	/** 构造函数
	*/
	ScheduledTask();

	// 步骤在整顿饭流程中的开始秒数
	int32_t m_startSeconds;
	// 步骤在整顿饭流程中的结束秒数
	int32_t m_endSeconds;
};

/** 用户可以离开厨房或处理其他事情的大段空闲时间
*/
struct FreeBlock
{
	/** 构造函数
	*/
	FreeBlock();

	// 空闲时间开始秒数
	int32_t m_startSeconds;
	// 空闲时间结束秒数
	int32_t m_endSeconds;
};

/** 多个菜谱组合后的排程结果
*/
struct PlanResult
{
	/** 构造函数
	*/
	PlanResult();

	// 本次排程实际选中的菜谱列表
	std::vector<const Recipe*> m_recipes;
	// 按开始时间排序后的完整时间线
	std::vector<ScheduledTask> m_timeline;
	// 可自由离开厨房的大段时间
	std::vector<FreeBlock> m_freeBlocks;
	// 从开始到能吃上的总耗时秒数
	int32_t m_totalSeconds;
	// 用户需要持续操作的总秒数
	int32_t m_activeSeconds;
	// 大段可自由支配时间秒数
	int32_t m_freeSeconds;
	// 无法有效利用的边角等待时间秒数
	int32_t m_edgeSeconds;
};

/** 简易账号状态，当前用于金币、购买菜谱和收藏菜谱
*/
struct UserAccount
{
	/** 构造函数
	*/
	UserAccount();

	// 用户ID，简易版未登录时默认使用demo_user
	std::string m_userId;
	// 账户金币余额
	int32_t m_coins;
	// 账号已经拥有的菜谱ID集合，免费默认菜和付费购买菜都放在这里
	std::set<std::string> m_ownedRecipeIds;
	// 已付费购买的菜谱ID集合，用于展示购买记录和后续收益统计
	std::set<std::string> m_purchasedRecipeIds;
	// 收藏菜谱ID集合
	std::set<std::string> m_favoriteRecipeIds;
};