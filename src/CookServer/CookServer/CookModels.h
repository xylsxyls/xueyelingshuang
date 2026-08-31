#pragma once
#include <map>
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
	// 步骤短ID，作者上传时在单个菜谱内填写，便于个性化配置按短ID匹配
	std::string m_shortId;
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
	// 后台等待性质，free表示可离开厨房，watch表示需要留在厨房守候
	std::string m_backgroundWaitMode;
	// 依赖步骤ID列表，当前步骤必须等这些步骤完成后才能开始
	std::vector<std::string> m_dependencies;
	// 步骤持续秒数，包含等待和操作时间
	int32_t m_durationSeconds;
	// 是否需要用户持续动手操作
	bool m_active;
	// 是否允许用户在制作页手动跳过
	bool m_manualSkippable;
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
	// 作者用户ID，系统菜谱为空或system
	std::string m_authorUserId;
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
	// 价格类型，free/coin/yuanbao
	std::string m_priceType;
	// 价格数量，priceType为coin或yuanbao时生效
	int32_t m_priceAmount;
	// 菜谱状态，published/draft/reviewing/rejected
	std::string m_status;
	// 当前公开版本ID，用户个性化和历史记录都绑定到该版本
	std::string m_currentRevisionId;
	// 新账号是否默认拥有该菜谱，真正的账号拥有关系仍以UserAccount中的菜谱ID集合为准
	bool m_defaultOwned;
	// 是否为系统内置菜谱
	bool m_systemRecipe;
	// 是否为用户上传菜谱
	bool m_customRecipe;
	// 当前返回给客户端的菜谱是否已应用用户个性化配置
	bool m_personalizationApplied;
	// 个性化摘要，用于前端提示“已按你的设置调整”
	std::string m_personalizationSummary;
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
	/** 拷贝构造函数，保证m_recipes在保存临时菜谱副本时仍指向当前对象
	*/
	PlanResult(const PlanResult& other);
	/** 赋值函数，保证m_recipes在保存临时菜谱副本时仍指向当前对象
	*/
	PlanResult& operator=(const PlanResult& other);

	// 本次排程实际选中的菜谱列表
	std::vector<const Recipe*> m_recipes;
	// 当排程基于临时菜谱副本时，副本保存在这里以保证m_recipes指针有效
	std::vector<Recipe> m_recipeCopies;
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
	// 排程提示或非致命告警，例如已应用个性化配置
	std::vector<std::string> m_warnings;
};

/** 用户账号状态，包含登录、钱包、菜谱关系和社交关系
*/
struct UserAccount
{
	/** 构造函数
	*/
	UserAccount();

	// 用户ID，服务端生成的稳定账号标识
	std::string m_userId;
	// 登录账号名，注册和登录时由用户填写
	std::string m_account;
	// 昵称，用于信息流、评论和创作者展示
	std::string m_nickname;
	// 密码盐，当前第一期使用本地模拟存储，后续接MySQL时迁移到用户表
	std::string m_passwordSalt;
	// 密码哈希，服务端不保存明文密码
	std::string m_passwordHash;
	// 账户金币余额
	int32_t m_coins;
	// 账户元宝余额，用于测试非金币售卖类型
	int32_t m_yuanbao;
	// 上次签到日期，格式YYYYMMDD
	std::string m_lastCheckinDate;
	// 账号创建时间，Unix秒
	int32_t m_createdAtSeconds;
	// 最近登录时间，Unix秒
	int32_t m_lastLoginAtSeconds;
	// 是否记录该用户的菜谱个性化配置
	bool m_personalizationRecordEnabled;
	// 账号已经拥有的菜谱ID集合，免费默认菜和付费购买菜都放在这里
	std::set<std::string> m_ownedRecipeIds;
	// 已付费购买的菜谱ID集合，用于展示购买记录和后续收益统计
	std::set<std::string> m_purchasedRecipeIds;
	// 收藏菜谱ID集合
	std::set<std::string> m_favoriteRecipeIds;
	// 收藏目标集合，格式targetType:targetId，兼容后续视频和动态收藏
	std::set<std::string> m_favoriteTargetIds;
	// 点赞目标集合，格式targetType:targetId
	std::set<std::string> m_likedTargetIds;
	// 已看过视频的时间戳，key为videoId，value为Unix秒
	std::map<std::string, int32_t> m_watchedVideoAtSeconds;
	// 当前账号关注的用户ID集合
	std::set<std::string> m_followingUserIds;
	// 当前账号发布的自定义菜谱ID集合
	std::set<std::string> m_customRecipeIds;
};

/** 用户关系摘要，用于前端展示关注和好友列表
*/
struct UserContactInfo
{
	/** 构造函数
	*/
	UserContactInfo();

	// 用户ID
	std::string m_userId;
	// 登录账号名
	std::string m_account;
	// 昵称
	std::string m_nickname;
	// 当前账号是否关注该用户
	bool m_following;
	// 双方是否互相关注
	bool m_friend;
	// 该用户关注的人数
	int32_t m_followingCount;
	// 该用户的粉丝人数
	int32_t m_followerCount;
};

/** 登录会话，服务端保存token哈希而不是明文token
*/
struct AuthSessionInfo
{
	/** 构造函数
	*/
	AuthSessionInfo();

	// token哈希
	std::string m_tokenHash;
	// 会话所属用户ID
	std::string m_userId;
	// 过期时间，Unix秒
	int32_t m_expireAtSeconds;
};

/** 广告奖励记录，保证同一次广告只能领取一次奖励
*/
struct AdRewardInfo
{
	/** 构造函数
	*/
	AdRewardInfo();

	// 广告奖励token
	std::string m_adToken;
	// 用户ID
	std::string m_userId;
	// 创建时间，Unix秒
	int32_t m_createdAtSeconds;
	// 奖励金币数
	int32_t m_rewardCoins;
	// 是否已经领取奖励
	bool m_finished;
};

/** 评论信息，第一期支持菜谱和视频评论
*/
struct CommentInfo
{
	/** 构造函数
	*/
	CommentInfo();

	// 评论ID
	std::string m_id;
	// 评论目标类型，recipe/video
	std::string m_targetType;
	// 评论目标ID
	std::string m_targetId;
	// 评论用户ID
	std::string m_userId;
	// 评论用户昵称快照
	std::string m_nickname;
	// 评论内容
	std::string m_content;
	// 评分，菜谱购买后评论时可传1-5，0表示无评分
	int32_t m_score;
	// 创建时间，Unix秒
	int32_t m_createdAtSeconds;
};

/** 站内消息
*/
struct MessageInfo
{
	/** 构造函数
	*/
	MessageInfo();

	// 消息ID
	std::string m_id;
	// 发送者用户ID，系统消息为system
	std::string m_fromUserId;
	// 接收者用户ID
	std::string m_toUserId;
	// 消息类型，system/follow/comment/recipe/chat
	std::string m_type;
	// 标题
	std::string m_title;
	// 正文
	std::string m_text;
	// 创建时间，Unix秒
	int32_t m_createdAtSeconds;
	// 是否已读
	bool m_read;
};

/** 视频或图文动态信息
*/
struct VideoInfo
{
	/** 构造函数
	*/
	VideoInfo();

	// 视频ID
	std::string m_id;
	// 发布用户ID
	std::string m_ownerUserId;
	// 媒体类型，video/image
	std::string m_mediaType;
	// 服务端保存路径或临时占位路径
	std::string m_filePath;
	// 发布文案
	std::string m_caption;
	// 挂载菜谱ID列表
	std::vector<std::string> m_recipeIds;
	// 状态，uploaded/published
	std::string m_status;
	// 创建时间，Unix秒
	int32_t m_createdAtSeconds;
};

/** 用户对单个菜谱的个性化排程配置
*/
struct PersonalizationConfig
{
	/** 构造函数
	*/
	PersonalizationConfig();

	// 配置所属用户ID
	std::string m_userId;
	// 菜谱ID
	std::string m_recipeId;
	// 被隐藏的步骤ID或短ID集合
	std::set<std::string> m_disabledTaskIds;
	// 步骤时长覆盖，key可为完整步骤ID或短ID
	std::map<std::string, int32_t> m_durationOverrides;
	// 用户新增步骤
	std::vector<CookTask> m_addedTasks;
	// 更新时间，Unix秒
	int32_t m_updatedAtSeconds;
};