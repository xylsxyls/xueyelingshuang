#pragma once
#include "AccountStore.h"
#include "CookModels.h"
#include "HttpService/HttpServiceAPI.h"

/** CookServer的HTTP业务接口层，负责把请求路由到菜谱、账号和排程逻辑
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
	/** 从查询参数或JSON请求体中提取用户ID
	@param [in] request HTTP请求
	@return 返回用户ID，缺省时返回demo_user
	*/
	std::string requestUserId(const HttpRequest& request) const;

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

	/** 构建健康检查JSON
	@return 返回健康检查响应体
	*/
	std::string buildHealthJson() const;

	/** 构建菜单菜谱列表JSON
	@param [in] userId 用户ID
	@return 返回菜谱列表、分类和账号状态
	*/
	std::string buildRecipesJson(const std::string& userId);

	/** 构建首页信息流JSON
	@param [in] userId 用户ID
	@return 返回示例视频流、点赞评论数和关联菜谱
	*/
	std::string buildFeedJson(const std::string& userId);

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

	/** 构建购买菜谱结果JSON
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@return 返回购买结果、账号状态和菜谱状态
	*/
	std::string buildPurchaseJson(const std::string& userId, const std::string& recipeId);

	/** 构建收藏切换结果JSON
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@return 返回收藏状态、账号状态和菜谱状态
	*/
	std::string buildFavoriteToggleJson(const std::string& userId, const std::string& recipeId);

	/** 构建做菜排程JSON
	@param [in] ids 菜谱ID列表
	@param [in] userId 用户ID
	@param [in] includePlanId 是否生成正式开始制作时使用的planId
	@return 返回排程结果或错误说明
	*/
	std::string buildPlanJson(const std::vector<std::string>& ids, const std::string& userId, bool includePlanId);

private:
	// 账号状态仓库，简易版使用本地文件持久化
	AccountStore m_accountStore;
};