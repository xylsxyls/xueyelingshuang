#pragma once
#include "CookModels.h"
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

/** 账号和业务状态仓库，第一期由CookStorageService承接Redis+MySQL快照存储
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
	@return 返回true表示账号已经拥有该菜谱
	*/
	static bool isRecipeOwned(const Recipe& recipe, const UserAccount& account);

	/** 注册账号并创建登录会话
	@param [in] accountName 登录账号名
	@param [in] password 登录密码
	@param [in] nickname 昵称
	@param [out] account 注册成功后的账号快照
	@param [out] token 登录token
	@param [out] expireInSeconds token剩余有效秒数
	@param [out] message 结果说明
	@return 返回是否注册成功
	*/
	bool registerAccount(const std::string& accountName,
	                     const std::string& password,
	                     const std::string& nickname,
	                     UserAccount& account,
	                     std::string& token,
	                     int32_t& expireInSeconds,
	                     std::string& message);

	/** 登录账号并创建登录会话
	@param [in] accountName 登录账号名
	@param [in] password 登录密码
	@param [out] account 登录成功后的账号快照
	@param [out] token 登录token
	@param [out] expireInSeconds token剩余有效秒数
	@param [out] message 结果说明
	@return 返回是否登录成功
	*/
	bool loginAccount(const std::string& accountName,
	                  const std::string& password,
	                  UserAccount& account,
	                  std::string& token,
	                  int32_t& expireInSeconds,
	                  std::string& message);

	/** 校验登录token
	@param [in] token 客户端传入的明文token
	@param [out] userId token所属用户ID
	@param [out] account token所属账号快照
	@param [out] message 失败说明
	@return 返回token是否有效
	*/
	bool validateToken(const std::string& token, std::string& userId, UserAccount& account, std::string& message);

	/** 注销登录token
	@param [in] token 客户端传入的明文token
	@return 返回是否找到并删除了会话
	*/
	bool logout(const std::string& token);

	/** 获取账号信息，不存在时会创建兼容demo账号
	@param [in] userId 用户ID，空字符串会被归一化为demo_user
	@return 返回账号快照
	*/
	UserAccount getAccount(const std::string& userId);

	/** 给账号充值金币，兼容早期测试接口
	@param [in] userId 用户ID
	@param [in] coins 充值金币数，必须为正数且不能过大
	@param [out] ok 返回充值是否成功，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount recharge(const std::string& userId, int32_t coins, bool* ok, std::string* message);

	/** 加入或购买菜谱，支持金币、元宝和幂等键
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [in] idempotencyKey 幂等键，可为空
	@param [out] ok 返回操作是否成功
	@param [out] alreadyOwned 返回操作前是否已经拥有
	@param [out] message 结果说明
	@param [out] recipe 操作目标菜谱快照
	@return 返回操作后的账号快照
	*/
	UserAccount joinRecipe(const std::string& userId,
	                       const std::string& recipeId,
	                       const std::string& idempotencyKey,
	                       bool* ok,
	                       bool* alreadyOwned,
	                       std::string* message,
	                       Recipe* recipe);

	/** 购买菜谱，兼容早期接口，内部转为joinRecipe
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] ok 返回购买是否成功，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount purchaseRecipe(const std::string& userId, const std::string& recipeId, bool* ok, std::string* message);

	/** 执行每日签到
	@param [in] userId 用户ID
	@param [out] ok 返回操作是否成功
	@param [out] alreadyCheckedIn 返回今天是否已经签到过
	@param [out] message 结果说明
	@return 返回操作后的账号快照
	*/
	UserAccount checkin(const std::string& userId, bool* ok, bool* alreadyCheckedIn, std::string* message);

	/** 创建广告奖励token
	@param [in] userId 用户ID
	@param [out] adToken 广告奖励token
	@param [out] rewardCoins 奖励金币数
	@param [out] requiredSeconds 最短观看秒数
	@return 返回账号快照
	*/
	UserAccount startAdReward(const std::string& userId, std::string& adToken, int32_t& rewardCoins, int32_t& requiredSeconds);

	/** 完成广告奖励
	@param [in] userId 用户ID
	@param [in] adToken 广告奖励token
	@param [in] debugElapsedSeconds 测试用已观看秒数，负数表示使用真实时间
	@param [out] ok 返回操作是否成功
	@param [out] alreadyRewarded 返回是否已领取过
	@param [out] message 结果说明
	@return 返回操作后的账号快照
	*/
	UserAccount finishAdReward(const std::string& userId,
	                           const std::string& adToken,
	                           int32_t debugElapsedSeconds,
	                           bool* ok,
	                           bool* alreadyRewarded,
	                           std::string* message);

	/** 切换收藏状态，targetType为空时按recipe处理
	@param [in] userId 用户ID
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@param [out] ok 返回操作是否成功
	@param [out] favorite 返回操作后是否已收藏
	@param [out] message 结果说明
	@return 返回操作后的账号快照
	*/
	UserAccount toggleFavorite(const std::string& userId,
	                           const std::string& targetType,
	                           const std::string& targetId,
	                           bool* ok,
	                           bool* favorite,
	                           std::string* message);

	/** 切换菜谱收藏状态，兼容早期接口
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] ok 返回操作是否成功
	@param [out] favorite 返回操作后是否已收藏，可为空
	@param [out] message 返回给前端展示的结果说明，可为空
	@return 返回操作后的账号快照
	*/
	UserAccount toggleFavorite(const std::string& userId, const std::string& recipeId, bool* ok, bool* favorite, std::string* message);

	/** 切换点赞状态
	@param [in] userId 用户ID
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@param [out] liked 返回操作后是否已点赞
	@return 返回操作是否成功
	*/
	bool toggleLike(const std::string& userId, const std::string& targetType, const std::string& targetId, bool& liked);

	/** 切换关注状态
	@param [in] userId 用户ID
	@param [in] targetUserId 被关注用户ID
	@param [out] following 返回操作后是否已关注
	@param [out] message 结果说明
	@return 返回操作是否成功
	*/
	bool toggleFollow(const std::string& userId, const std::string& targetUserId, bool& following, std::string& message);

	/** 创建评论
	@param [in] userId 用户ID
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@param [in] content 评论内容
	@param [in] score 菜谱评分，0表示不评分
	@param [out] comment 创建后的评论
	@param [out] message 结果说明
	@return 返回操作是否成功
	*/
	bool createComment(const std::string& userId,
	                   const std::string& targetType,
	                   const std::string& targetId,
	                   const std::string& content,
	                   int32_t score,
	                   CommentInfo& comment,
	                   std::string& message);

	/** 查询评论列表
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@return 返回评论快照列表
	*/
	std::vector<CommentInfo> listComments(const std::string& targetType, const std::string& targetId);

	/** 查询站内消息
	@param [in] userId 用户ID
	@return 返回消息快照列表
	*/
	std::vector<MessageInfo> listMessages(const std::string& userId);

	/** 上传或登记视频元数据
	@param [in] userId 用户ID
	@param [in] mediaType 媒体类型
	@param [in] filePath 文件路径或占位路径
	@param [in] fileSizeBytes 文件字节数
	@param [out] video 创建的视频信息
	@param [out] message 结果说明
	@return 返回操作是否成功
	*/
	bool uploadVideo(const std::string& userId,
	                 const std::string& mediaType,
	                 const std::string& filePath,
	                 int32_t fileSizeBytes,
	                 VideoInfo& video,
	                 std::string& message);

	/** 发布视频并挂载菜谱
	@param [in] userId 用户ID
	@param [in] videoId 视频ID，可为空
	@param [in] caption 发布文案
	@param [in] recipeIds 挂载菜谱ID列表
	@param [out] video 发布后的视频信息
	@param [out] message 结果说明
	@return 返回操作是否成功
	*/
	bool publishVideo(const std::string& userId,
	                  const std::string& videoId,
	                  const std::string& caption,
	                  const std::vector<std::string>& recipeIds,
	                  VideoInfo& video,
	                  std::string& message);

	/** 发布或保存自定义菜谱
	@param [in] userId 用户ID
	@param [in] sourceRecipe 菜谱数据
	@param [in] published 是否发布
	@param [out] recipe 保存后的菜谱
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool saveCustomRecipe(const std::string& userId,
	                      const Recipe& sourceRecipe,
	                      bool published,
	                      Recipe& recipe,
	                      std::string& message);

	/** 保存菜谱个性化配置
	@param [in] userId 用户ID
	@param [in] config 个性化配置
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool savePersonalization(const std::string& userId, const PersonalizationConfig& config, std::string& message);

	/** 同步本次做菜实际耗时到菜谱个性化配置
	@param [in] userId 用户ID
	@param [in] recipeDurationOverrides 每道菜的步骤耗时覆盖，外层key为recipeId，内层key为步骤ID或短ID
	@param [out] updatedRecipeCount 更新的菜谱数量
	@param [out] updatedTaskCount 更新的步骤数量
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool syncPersonalizationDurations(const std::string& userId,
	                                  const std::map<std::string, std::map<std::string, int32_t>>& recipeDurationOverrides,
	                                  int32_t& updatedRecipeCount,
	                                  int32_t& updatedTaskCount,
	                                  std::string& message);

	/** 重置菜谱个性化配置
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool resetPersonalization(const std::string& userId, const std::string& recipeId, std::string& message);

	/** 分享菜谱个性化配置
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] shareId 分享ID
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool sharePersonalization(const std::string& userId, const std::string& recipeId, std::string& shareId, std::string& message);

	/** 应用他人分享的菜谱个性化配置
	@param [in] userId 用户ID
	@param [in] shareId 分享ID
	@param [out] config 应用后的配置
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool applySharedPersonalization(const std::string& userId, const std::string& shareId, PersonalizationConfig& config, std::string& message);

	/** 查询用户可见菜谱列表，已拥有菜谱会应用个性化配置
	@param [in] userId 用户ID
	@return 返回菜谱快照列表
	*/
	std::vector<Recipe> listRecipesForUser(const std::string& userId);

	/** 查询单个菜谱，已拥有菜谱会应用个性化配置
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [in] requireOwned 是否要求用户已拥有
	@param [out] recipe 菜谱快照
	@param [out] message 结果说明
	@return 返回是否找到并满足权限
	*/
	bool getRecipeForUser(const std::string& userId, const std::string& recipeId, bool requireOwned, Recipe& recipe, std::string& message);

	/** 查询单个菜谱的原始步骤和当前个性化配置，不应用禁用和耗时覆盖
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [out] recipe 原始菜谱快照
	@param [out] config 当前个性化配置，不存在时返回空配置
	@param [out] hasConfig 是否已有个性化配置
	@param [out] message 结果说明
	@return 返回是否找到并满足权限
	*/
	bool getPersonalizationDetail(const std::string& userId,
	                              const std::string& recipeId,
	                              Recipe& recipe,
	                              PersonalizationConfig& config,
	                              bool& hasConfig,
	                              std::string& message);

	/** 记录一次做菜结果
	@param [in] userId 用户ID
	@param [in] planId 排程ID
	@param [in] recipeIds 本次菜谱ID列表
	@param [in] imagePath 成果图片路径或占位值
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool recordCookResult(const std::string& userId,
	                      const std::string& planId,
	                      const std::vector<std::string>& recipeIds,
	                      const std::string& imagePath,
	                      std::string& message);

	/** 查询首页视频流，本期用未看过视频随机选择，后续推荐算法只替换这里
	@param [in] userId 用户ID
	@param [in] count 需要返回的视频数量
	@param [in] excludeVideoIds 客户端当前已持有的视频ID，避免一页内重复
	@param [in] mode 推荐模式，liked/favorite用于“我的”页筛选，空字符串表示推荐流
	@param [in] avoidVideoId 本次返回时尽量避开的当前视频ID
	@param [out] resetSeen 返回本次是否因为看完全部视频而重置了看过状态
	@return 返回视频列表
	*/
	std::vector<VideoInfo> listFeedVideosForUser(const std::string& userId,
	                                             int32_t count,
	                                             const std::set<std::string>& excludeVideoIds,
	                                             const std::string& mode,
	                                             const std::string& avoidVideoId,
	                                             bool& resetSeen);

	/** 标记用户已看过某视频
	@param [in] userId 用户ID
	@param [in] videoId 视频ID
	@param [out] watchedAtSeconds 标记时间
	@param [out] message 结果说明
	@return 返回是否成功
	*/
	bool markVideoWatched(const std::string& userId,
	                      const std::string& videoId,
	                      int32_t& watchedAtSeconds,
	                      std::string& message);

	/** 查询可播放视频文件
	@param [in] videoId 视频ID
	@param [out] video 视频元数据
	@param [out] filePath 本机文件路径
	@param [out] contentType HTTP内容类型
	@return 返回是否找到且文件存在
	*/
	bool getVideoFile(const std::string& videoId, VideoInfo& video, std::string& filePath, std::string& contentType);

	/** 统计目标点赞数
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@return 返回点赞数
	*/
	int32_t countTargetLikes(const std::string& targetType, const std::string& targetId);

	/** 统计目标评论数
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@return 返回评论数
	*/
	int32_t countTargetComments(const std::string& targetType, const std::string& targetId);

private:
	/** 归一化用户ID
	@param [in] userId 用户ID
	@return 返回可用于持久化的用户ID
	*/
	std::string normalizeUserId(const std::string& userId) const;

	/** 归一化登录账号
	@param [in] accountName 登录账号名
	@return 返回去掉首尾空白后的账号名
	*/
	std::string normalizeAccountNameNoLock(const std::string& accountName) const;

	/** 按分隔符拆分账号状态行，保留空字段，调用方必须已持有锁
	@param [in] text 待拆分文本
	@param [in] delimiter 分隔符
	@return 返回拆分后的字段列表
	*/
	std::vector<std::string> splitKeepEmptyNoLock(const std::string& text, char delimiter) const;

	/** 兼容旧版本内置菜谱ID，调用方必须已持有锁
	@param [in] recipeId 菜谱ID
	@return 返回当前版本使用的菜谱ID
	*/
	std::string normalizeRecipeIdNoLock(const std::string& recipeId) const;

	/** 批量插入菜谱ID，插入前会做旧版本ID兼容，调用方必须已持有锁
	@param [out] recipeIds 待写入的菜谱ID集合
	@param [in] sourceRecipeIds 原始菜谱ID列表
	*/
	void insertRecipeIdsNoLock(std::set<std::string>& recipeIds, const std::vector<std::string>& sourceRecipeIds) const;

	/** 给账号补齐默认拥有菜谱和历史购买菜谱，调用方必须已持有锁
	@param [in,out] account 待补齐账号
	*/
	void refreshOwnedRecipesNoLock(UserAccount& account) const;

	/** 在未加载时从Redis+MySQL读取账号数据，调用方必须已持有锁
	*/
	void loadIfNeededNoLock();

	/** 将内存中的账号数据写回Redis+MySQL，调用方必须已持有锁
	*/
	void saveNoLock();

	/** 确保账号存在，调用方必须已持有锁
	@param [in] userId 用户ID
	@return 返回可修改的账号引用
	*/
	UserAccount& ensureNoLock(const std::string& userId);

	/** 查找基础菜谱，优先查用户自定义菜谱，再查系统目录，调用方必须已持有锁
	@param [in] recipeId 菜谱ID
	@return 返回菜谱指针，未找到返回nullptr
	*/
	const Recipe* findBaseRecipeNoLock(const std::string& recipeId) const;

	/** 同步res/video目录中的视频到内存视频表，调用方必须已持有锁
	@param [in] force 是否忽略扫描间隔强制同步
	*/
	void syncResourceVideosNoLock(bool force);

	/** 将配置或数据库中的视频路径解析成本机绝对路径，调用方必须已持有锁
	@param [in] path 视频路径
	@return 返回本机文件路径
	*/
	std::string resolveVideoFilePathNoLock(const std::string& path) const;

	/** 清理已过期的视频观看记录，调用方必须已持有锁
	@param [in,out] account 用户账号
	@param [in] nowSeconds 当前Unix秒
	*/
	void pruneExpiredVideoSeenNoLock(UserAccount& account, int32_t nowSeconds) const;

	/** 计算视频推荐随机分数，调用方必须已持有锁
	@param [in] userId 用户ID
	@param [in] videoId 视频ID
	@param [in] seed 本轮随机种子
	@return 返回用于排序的分数文本
	*/
	std::string videoRecommendScoreNoLock(const std::string& userId, const std::string& videoId, const std::string& seed) const;

	/** 将用户个性化配置应用到菜谱副本，调用方必须已持有锁
	@param [in] userId 用户ID
	@param [in,out] recipe 菜谱副本
	*/
	void applyPersonalizationNoLock(const std::string& userId, Recipe& recipe) const;

	/** 构建存储用菜谱JSON
	@param [in] recipe 菜谱数据
	@return 返回JSON文本
	*/
	std::string recipeToStorageJsonNoLock(const Recipe& recipe) const;

	/** 构建存储用个性化JSON
	@param [in] config 个性化配置
	@return 返回JSON文本
	*/
	std::string personalizationToStorageJsonNoLock(const PersonalizationConfig& config) const;

	/** 从JSON解析个性化配置
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [in] json JSON文本
	@param [out] config 个性化配置
	@return 返回是否解析成功
	*/
	bool personalizationFromStorageJsonNoLock(const std::string& userId,
	                                          const std::string& recipeId,
	                                          const std::string& json,
	                                          PersonalizationConfig& config) const;

	/** 创建登录会话，调用方必须已持有锁
	@param [in] userId 用户ID
	@param [out] token 登录token
	@param [out] expireInSeconds token剩余有效秒数
	*/
	void createSessionNoLock(const std::string& userId, std::string& token, int32_t& expireInSeconds);

	/** 添加站内消息，调用方必须已持有锁
	@param [in] fromUserId 发送者用户ID
	@param [in] toUserId 接收者用户ID
	@param [in] type 消息类型
	@param [in] title 标题
	@param [in] text 正文
	*/
	void addMessageNoLock(const std::string& fromUserId,
	                      const std::string& toUserId,
	                      const std::string& type,
	                      const std::string& title,
	                      const std::string& text);

	/** 获取当前Unix秒
	@return 返回当前时间
	*/
	int32_t nowSecondsNoLock() const;

	/** 获取当前本地日期
	@return 返回YYYYMMDD格式日期
	*/
	std::string todayTextNoLock() const;

	/** 构造内部ID
	@param [in] prefix ID前缀
	@return 返回唯一性足够的ID
	*/
	std::string makeIdNoLock(const std::string& prefix);

	/** 构造哈希文本
	@param [in] text 待哈希文本
	@return 返回十六进制哈希
	*/
	std::string hashTextNoLock(const std::string& text) const;

	/** 替换不适合写入制表符快照文本的字符
	@param [in] text 原始文本
	@return 返回安全字段文本
	*/
	std::string safeFieldNoLock(const std::string& text) const;

	/** 生成目标集合key
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@return 返回集合key
	*/
	std::string targetKeyNoLock(const std::string& targetType, const std::string& targetId) const;

private:
	// 保护仓库内存表和加载状态
	std::mutex m_mutex;
	// 已加载的账号表，key为用户ID
	std::map<std::string, UserAccount> m_accounts;
	// 登录账号到用户ID索引
	std::map<std::string, std::string> m_accountIndex;
	// token哈希到登录会话索引
	std::map<std::string, AuthSessionInfo> m_sessions;
	// 广告奖励记录
	std::map<std::string, AdRewardInfo> m_adRewards;
	// 用户自定义菜谱
	std::map<std::string, Recipe> m_customRecipes;
	// 评论列表
	std::vector<CommentInfo> m_comments;
	// 站内消息列表
	std::vector<MessageInfo> m_messages;
	// 视频元数据表
	std::map<std::string, VideoInfo> m_videos;
	// 是否已经同步过res/video资源目录
	bool m_resourceVideosSynced;
	// 上次同步res/video资源目录的Unix秒
	int32_t m_lastResourceVideoSyncSeconds;
	// 用户个性化配置，key为userId|recipeId
	std::map<std::string, PersonalizationConfig> m_personalizations;
	// 已分享个性化配置，key为shareId
	std::map<std::string, PersonalizationConfig> m_sharedPersonalizations;
	// 成功幂等键集合
	std::set<std::string> m_idempotencyKeys;
	// 内部ID序列
	int32_t m_sequence;
	// 是否已经尝试加载过Redis+MySQL状态快照
	bool m_loaded;
};
