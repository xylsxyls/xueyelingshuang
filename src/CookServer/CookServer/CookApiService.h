#pragma once
#include "AccountStore.h"
#include "CookModels.h"
#include "HttpService/HttpServiceAPI.h"
#include <deque>
#include <map>
#include <mutex>
#include <string>

class RapidJsonDocument;
class RapidJsonValue;

/** CookServer的HTTP业务接口层，负责把请求路由到菜谱、账号、钱包、内容和排程逻辑
*/
class CookApiService
{
public:
	/** 处理一条HTTP请求
	@param [in] request HttpService解析出的请求对象
	@return 返回HTTP响应，响应体当前统一为JSON字符串
	*/
	HttpResponse handle(const HttpRequest& request);

private:
	struct VideoPosterCacheEntry
	{
		std::string m_body;
		std::string m_contentType;
	};

	/** 从HTTP请求头读取字段，忽略大小写
	@param [in] request HTTP请求
	@param [in] headerName 请求头名称
	@return 返回请求头内容，不存在时返回空字符串
	*/
	std::string headerValue(const HttpRequest& request, const std::string& headerName) const;

	/** 从Authorization头中提取Bearer token
	@param [in] request HTTP请求
	@return 返回token，不存在时返回空字符串
	*/
	std::string bearerToken(const HttpRequest& request) const;

	/** 解析JSON请求体
	@param [in] body HTTP请求体
	@param [out] document JSON文档
	@return 返回是否为合法JSON对象
	*/
	bool parseBody(const std::string& body, RapidJsonDocument& document) const;

	/** 从JSON请求体中解析菜谱ID列表
	@param [in] body HTTP请求体
	@return 返回菜谱ID列表，解析失败时返回空列表
	*/
	std::vector<std::string> parseRecipeIds(const std::string& body) const;

	/** 从JSON请求体中读取字符串字段
	@param [in] body HTTP请求体
	@param [in] key 字段名
	@return 返回字段值，字段不存在或解析失败时返回空字符串
	*/
	std::string jsonFieldString(const std::string& body, const char* key) const;

	/** 从JSON请求体中读取整数字段
	@param [in] body HTTP请求体
	@param [in] key 字段名
	@param [in] defaultValue 解析失败或字段不存在时的默认值
	@return 返回字段值或默认值
	*/
	int32_t jsonFieldInt(const std::string& body, const char* key, int32_t defaultValue) const;

	/** 从JSON请求体中读取布尔字段
	@param [in] body HTTP请求体
	@param [in] key 字段名
	@param [in] defaultValue 解析失败或字段不存在时的默认值
	@return 返回字段值或默认值
	*/
	bool jsonFieldBool(const std::string& body, const char* key, bool defaultValue) const;

	/** 校验登录态
	@param [in] request HTTP请求
	@param [out] userId 登录用户ID
	@param [out] account 登录账号快照
	@param [out] response 认证失败时的HTTP响应
	@return 返回认证是否通过
	*/
	bool authorize(const HttpRequest& request, std::string& userId, UserAccount& account, HttpResponse& response);

	/** 构建错误JSON响应
	@param [in] message 错误说明
	@param [in] statusCode HTTP状态码
	@param [in] errorCode 业务错误码
	@return 返回HTTP响应
	*/
	HttpResponse errorResponse(const std::string& message, int32_t statusCode, const std::string& errorCode) const;

	/** 构建健康检查JSON
	@return 返回健康检查响应体
	*/
	std::string buildHealthJson() const;

	/** 构建注册结果JSON
	@param [in] body HTTP请求体
	@return 返回注册响应体
	*/
	std::string buildRegisterJson(const std::string& body);

	/** 构建登录结果JSON
	@param [in] body HTTP请求体
	@return 返回登录响应体
	*/
	std::string buildLoginJson(const std::string& body);

	/** 构建会话JSON
	@param [in] account 账号快照
	@return 返回会话响应体
	*/
	std::string buildSessionJson(const UserAccount& account) const;

	/** 判断菜谱是否属于菜单中的某个分类栏
	@param [in] recipe 菜谱
	@param [in] account 当前账号
	@param [in] categoryName 菜单分类栏名称
	@return 返回是否属于该分类栏
	*/
	bool recipeMatchesMenuCategory(const Recipe& recipe, const UserAccount& account, const std::string& categoryName) const;

	/** 构建菜单菜谱列表JSON
	@param [in] userId 用户ID
	@param [in] request HTTP请求，用于读取分包参数
	@return 返回菜谱列表、分类和账号状态
	*/
	std::string buildRecipesJson(const std::string& userId);

	/** 构建菜谱详情JSON
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@return 返回菜谱详情
	*/
	std::string buildRecipeDetailJson(const std::string& userId, const std::string& recipeId);

	/** 构建菜谱个性化详情JSON，返回原始步骤和当前个性化配置
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@return 返回个性化详情
	*/
	std::string buildPersonalizationDetailJson(const std::string& userId, const std::string& recipeId);

	/** 构建首页信息流JSON
	@param [in] request HTTP请求
	@param [in] userId 用户ID
	@return 返回真实视频流、点赞评论数和关联菜谱
	*/
	std::string buildFeedJson(const HttpRequest& request, const std::string& userId);

	/** 构建视频已观看上报JSON
	@param [in] userId 用户ID
	@param [in] body 请求体
	@return 返回上报结果
	*/
	std::string buildFeedWatchJson(const std::string& userId, const std::string& body);

	/** 构建账号信息JSON
	@param [in] userId 用户ID
	@return 返回账号金币、购买和收藏状态
	*/
	std::string buildAccountJson(const std::string& userId);

	/** 构建充值结果JSON
	@param [in] userId 用户ID
	@param [in] coins 充值金币数
	@return 返回充值结果和账号状态
	*/
	std::string buildRechargeJson(const std::string& userId, int32_t coins);

	/** 构建加入菜谱结果JSON
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@param [in] idempotencyKey 幂等键
	@return 返回加入结果、账号状态和菜谱状态
	*/
	std::string buildJoinRecipeJson(const std::string& userId, const std::string& recipeId, const std::string& idempotencyKey);

	/** 构建收藏切换结果JSON
	@param [in] userId 用户ID
	@param [in] targetType 目标类型
	@param [in] targetId 目标ID
	@return 返回收藏状态、账号状态和菜谱状态
	*/
	std::string buildFavoriteToggleJson(const std::string& userId, const std::string& targetType, const std::string& targetId);

	/** 构建做菜排程JSON
	@param [in] ids 菜谱ID列表
	@param [in] userId 用户ID
	@param [in] includePlanId 是否生成正式开始制作时使用的planId
	@param [in] planId 已生成的排程ID，空字符串表示本次生成
	@param [in] summaryOnly 是否只返回菜单预览摘要
	@param [in] includeTimeBlocks 摘要模式下是否返回自由、守候和时间条区间
	@param [in] lite 是否只返回步骤页启动必需字段
	@return 返回排程结果或错误说明
	*/
	std::string buildPlanJson(const std::vector<std::string>& ids, const std::string& userId, bool includePlanId, const std::string& planId, bool summaryOnly, bool includeTimeBlocks, bool lite);

	/** 构建服务端语音合成JSON
	@param [in] request HTTP请求，用于生成同源音频URL
	@return 返回语音合成结果
	*/
	std::string buildVoiceTtsJson(const HttpRequest& request);

	/** 构建语音准备JSON
	@param [in] request HTTP请求
	@return 返回语音ID列表和初始必备语音条
	*/
	std::string buildVoicePrepareJson(const HttpRequest& request);

	/** 构建按voiceId拉取语音JSON
	@param [in] request HTTP请求
	@return 返回语音状态或音频信息
	*/
	std::string buildVoiceFetchJson(const HttpRequest& request);

	/** 构建语音音频文件响应
	@param [in] request HTTP请求
	@return 返回音频二进制响应
	*/
	HttpResponse buildVoiceAudioResponse(const HttpRequest& request) const;

	/** 构建视频文件响应，支持Range分段读取
	@param [in] request HTTP请求
	@return 返回视频二进制响应
	*/
	HttpResponse buildVideoFileResponse(const HttpRequest& request);

	/** 构建视频第一帧图片响应，通过FFmpegCpp在内存中生成
	@param [in] request HTTP请求
	@return 返回图片二进制响应
	*/
	HttpResponse buildVideoPosterResponse(const HttpRequest& request);

	/** 从内存读取视频第一帧图片缓存
	@param [in] videoId 视频ID
	@param [out] body 图片二进制
	@param [out] contentType 图片MIME
	@return 返回是否命中缓存
	*/
	bool readVideoPosterCache(const std::string& videoId, std::string& body, std::string& contentType);

	/** 写入视频第一帧图片内存缓存
	@param [in] videoId 视频ID
	@param [in] body 图片二进制
	@param [in] contentType 图片MIME
	*/
	void writeVideoPosterCache(const std::string& videoId, const std::string& body, const std::string& contentType);

	/** 基于当前请求生成API根地址
	@param [in] request HTTP请求
	@return 返回http或https根地址
	*/
	std::string buildRequestBaseUrl(const HttpRequest& request) const;

	/** 构建视频文件URL
	@param [in] request HTTP请求
	@param [in] videoId 视频ID
	@return 返回同源视频URL
	*/
	std::string buildVideoFileUrl(const HttpRequest& request, const std::string& videoId) const;

	/** 构建视频第一帧图片URL
	@param [in] request HTTP请求
	@param [in] videoId 视频ID
	@return 返回同源第一帧图片URL
	*/
	std::string buildVideoPosterUrl(const HttpRequest& request, const std::string& videoId) const;

	/** 解析视频Range请求头
	@param [in] rangeHeader Range头
	@param [in] fileSize 文件大小
	@param [out] startByte 起始字节
	@param [out] endByte 结束字节
	@return 返回是否解析成功
	*/
	bool parseVideoRange(const std::string& rangeHeader, int64_t fileSize, int64_t& startByte, int64_t& endByte) const;

	/** 从请求体解析个性化配置
	@param [in] body HTTP请求体
	@param [in] userId 用户ID
	@param [out] config 个性化配置
	@param [out] message 失败说明
	@return 返回是否解析成功
	*/
	bool parsePersonalizationConfig(const std::string& body, const std::string& userId, PersonalizationConfig& config, std::string& message) const;

	/** 将个性化配置转为JSON对象
	@param [in] config 个性化配置
	@param [in] hasConfig 是否已有配置
	@return 返回JSON对象
	*/
	RapidJsonValue personalizationConfigToJson(const PersonalizationConfig& config, bool hasConfig) const;

	/** 将个性化新增步骤转为JSON对象
	@param [in] task 菜谱步骤
	@return 返回JSON对象
	*/
	RapidJsonValue personalizationTaskToJson(const CookTask& task) const;

	/** 将评论转为JSON
	@param [in] comment 评论
	@return 返回JSON对象
	*/
	RapidJsonValue commentToJson(const CommentInfo& comment) const;

	/** 将消息转为JSON
	@param [in] message 消息
	@return 返回JSON对象
	*/
	RapidJsonValue messageToJson(const MessageInfo& message) const;

	/** 将用户关系摘要转为JSON
	@param [in] contact 用户关系摘要
	@return 返回前端可展示的用户关系对象
	*/
	RapidJsonValue userContactToJson(const UserContactInfo& contact) const;

	/** 将用户关系摘要列表转为JSON数组
	@param [in] contacts 用户关系摘要列表
	@return 返回前端可展示的用户关系数组
	*/
	RapidJsonValue userContactListToJson(const std::vector<UserContactInfo>& contacts) const;

	/** 将视频信息转为JSON
	@param [in] video 视频信息
	@return 返回JSON对象
	*/
	RapidJsonValue videoToJson(const VideoInfo& video) const;

	/** 将视频信息转为首页信息流卡片JSON
	@param [in] request HTTP请求
	@param [in] video 视频信息
	@param [in] account 当前账号
	@param [in] recipesById 当前用户可见菜谱索引
	@param [in] index 当前结果序号
	@return 返回首页信息流卡片
	*/
	RapidJsonValue feedVideoToJson(const HttpRequest& request,
	                               const VideoInfo& video,
	                               const UserAccount& account,
	                               const std::map<std::string, Recipe>& recipesById,
	                               int32_t index);

private:
	// 账号、钱包、内容和用户菜谱状态仓库
	AccountStore m_accountStore;
	// 视频第一帧图片内存缓存，避免重复解析同一个视频文件
	std::mutex m_videoPosterCacheMutex;
	std::map<std::string, VideoPosterCacheEntry> m_videoPosterCache;
	std::deque<std::string> m_videoPosterCacheOrder;
};