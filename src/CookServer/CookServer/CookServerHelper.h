#pragma once
#include "CookModels.h"
#include "HttpService/HttpServiceAPI.h"
#include "RapidJson/RapidJsonAPI.h"
#include <stdint.h>
#include <set>
#include <string>
#include <vector>

/** CookServer通用辅助函数集合，放置跨业务类复用但不属于某个业务类本体的逻辑
*/
class CookServerHelper
{
public:
	/** 按单字符分隔字符串，空片段会被忽略
	@param [in] text 待分隔字符串
	@param [in] delimiter 分隔符
	@return 返回分隔后的非空片段
	*/
	static std::vector<std::string> split(const std::string& text, char delimiter);

	/** 将字符串集合拼接成一行可持久化文本
	@param [in] values 待拼接集合
	@param [in] delimiter 分隔符
	@return 返回拼接后的字符串
	*/
	static std::string joinSet(const std::set<std::string>& values, char delimiter);

	/** 将秒数向上折算成分钟，用于前端展示预估耗时
	@param [in] seconds 秒数
	@return 返回向上取整后的分钟数
	*/
	static int32_t ceilMinutes(int32_t seconds);

	/** 判断文本是否是合法UTF-8编码
	@param [in] text 待判断文本
	@return 合法UTF-8返回true，否则返回false
	*/
	static bool isUtf8Text(const std::string& text);

	/** 将业务文本转换成JSON响应使用的UTF-8文本
	@param [in] text 业务层文本，Windows下通常是本地Ansi编码，Linux下通常是UTF-8
	@return 返回适合写入JSON响应体的UTF-8文本
	*/
	static std::string textToJsonUtf8(const std::string& text);

	/** 拼接两段业务文本并统一转换成JSON响应使用的UTF-8文本
	@param [in] first 第一段文本
	@param [in] second 第二段文本
	@return 返回拼接后的UTF-8文本
	*/
	static std::string joinJsonUtf8Text(const std::string& first, const std::string& second);

	/** 拼接三段业务文本并统一转换成JSON响应使用的UTF-8文本
	@param [in] first 第一段文本
	@param [in] second 第二段文本
	@param [in] third 第三段文本
	@return 返回拼接后的UTF-8文本
	*/
	static std::string joinJsonUtf8Text(const std::string& first, const std::string& second, const std::string& third);

	/** 向JSON对象写入字符串字段
	@param [in,out] object JSON对象
	@param [in] key 字段名
	@param [in] text 字段值
	*/
	static void addString(RapidJsonValue& object, const char* key, const std::string& text);

	/** 向JSON根文档写入字符串字段
	@param [in,out] object JSON文档
	@param [in] key 字段名
	@param [in] text 字段值
	*/
	static void addString(RapidJsonDocument& object, const char* key, const std::string& text);

	/** 向JSON对象写入整数字段
	@param [in,out] object JSON对象
	@param [in] key 字段名
	@param [in] number 字段值
	*/
	static void addInt(RapidJsonValue& object, const char* key, int32_t number);

	/** 向JSON根文档写入整数字段
	@param [in,out] object JSON文档
	@param [in] key 字段名
	@param [in] number 字段值
	*/
	static void addInt(RapidJsonDocument& object, const char* key, int32_t number);

	/** 向JSON对象写入布尔字段
	@param [in,out] object JSON对象
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	static void addBool(RapidJsonValue& object, const char* key, bool value);

	/** 向JSON根文档写入布尔字段
	@param [in,out] object JSON文档
	@param [in] key 字段名
	@param [in] value 字段值
	*/
	static void addBool(RapidJsonDocument& object, const char* key, bool value);

	/** 向JSON数组追加字符串元素
	@param [in,out] array JSON数组
	@param [in] text 字符串元素值
	*/
	static void pushString(RapidJsonValue& array, const std::string& text);

	/** 向JSON对象写入字符串数组字段
	@param [in,out] object JSON对象
	@param [in] key 字段名
	@param [in] values 字符串数组
	*/
	static void addStringArray(RapidJsonValue& object, const char* key, const std::vector<std::string>& values);

	/** 将账号状态转换成JSON对象
	@param [in] account 账号状态
	@return 返回JSON对象
	*/
	static RapidJsonValue accountToJson(const UserAccount& account);

	/** 将菜谱转换成JSON对象
	@param [in] recipe 菜谱数据
	@param [in] account 当前账号，可为空
	@return 返回JSON对象
	*/
	static RapidJsonValue recipeToJson(const Recipe& recipe, const UserAccount* account);

	/** 将排程结果转换成JSON对象
	@param [in] plan 排程结果
	@param [in] account 当前账号
	@param [in] includePlanId 是否写入正式开始制作时的planId
	@return 返回JSON对象
	*/
	static RapidJsonValue planToJson(const PlanResult& plan, const UserAccount& account, bool includePlanId);

	/** 构建JSON HTTP响应
	@param [in] body JSON响应体
	@param [in] statusCode HTTP状态码
	@return 返回HttpService响应对象
	*/
	static HttpResponse jsonResponse(const std::string& body, int32_t statusCode = kHttpStatusOk);

	/** 按下标获取CookServer注册的API路由
	@param [in] index 路由下标
	@return 返回路由字符串，下标越界时返回空字符串
	*/
	static const char* routeAt(size_t index);

	/** 获取CookServer需要注册的API路由数量
	@return 返回路由数量
	*/
	static size_t routeCount();
};