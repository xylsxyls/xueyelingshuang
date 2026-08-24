#pragma once
#include "CookServerTestHttpClient.h"
#include "CookServerTestResult.h"
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

/** CookServer黑盒测试执行器，负责功能测试、压力测试和报告输出
*/
class CookServerTestRunner
{
public:
	/** 构造函数，初始化测试执行器
	*/
	CookServerTestRunner();

	/** 析构函数，测试执行器不持有外部资源
	*/
	~CookServerTestRunner();

	/** 执行全部测试并写入报告
	@return 返回是否全部测试通过
	*/
	bool run();

private:
	/** 执行单个功能测试用例并记录结果
	@param [in] name 用例名称
	@param [in] testFunction 用例函数，返回是否通过
	*/
	void runCase(const std::string& name, const std::function<bool(std::string&, int32_t&)>& testFunction);

	/** 执行所有功能测试
	*/
	void runFunctionalTests();

	/** 执行并发压力测试
	*/
	void runStressTests();

	/** 执行单个压力请求
	@param [in] client HTTP客户端
	@param [in] threadIndex 线程下标
	@param [in] loopIndex 循环下标
	@param [out] costMs 请求耗时
	@param [out] error 失败原因
	@return 返回请求是否符合预期
	*/
	bool executeStressRequest(CookServerTestHttpClient& client, int32_t threadIndex, int32_t loopIndex, int32_t& costMs, std::string& error) const;

	/** 记录单个压力请求结果
	@param [in] ok 请求是否成功
	@param [in] costMs 请求耗时
	@param [in] message 失败消息
	*/
	void recordStressResult(bool ok, int32_t costMs, const std::string& message);

	/** 根据HTTP响应校验网络层是否成功
	@param [in] response HTTP响应
	@param [out] message 失败消息
	@return 返回是否成功
	*/
	bool expectNetworkOk(const CookServerTestHttpResponse& response, std::string& message) const;

	/** 校验HTTP状态码
	@param [in] response HTTP响应
	@param [in] expectedStatus 期望状态码
	@param [out] message 失败消息
	@return 返回是否符合预期
	*/
	bool expectStatus(const CookServerTestHttpResponse& response, int32_t expectedStatus, std::string& message) const;

	/** 校验JSON根对象ok字段
	@param [in] response HTTP响应
	@param [in] expectedOk 期望ok值
	@param [out] message 失败消息
	@return 返回是否符合预期
	*/
	bool expectJsonOk(const CookServerTestHttpResponse& response, bool expectedOk, std::string& message) const;

	/** 读取JSON根对象ok字段
	@param [in] response HTTP响应
	@return 返回ok字段值，解析失败时返回false
	*/
	bool responseJsonOk(const CookServerTestHttpResponse& response) const;

	/** 校验响应体包含关键文本
	@param [in] response HTTP响应
	@param [in] text 期望文本
	@param [out] message 失败消息
	@return 返回是否包含
	*/
	bool expectBodyContains(const CookServerTestHttpResponse& response, const std::string& text, std::string& message) const;

	/** 从JSON响应体读取字符串字段
	@param [in] response HTTP响应
	@param [in] key 字段名
	@return 返回字段值，解析失败时返回空字符串
	*/
	std::string responseJsonString(const CookServerTestHttpResponse& response, const char* key) const;

	/** 构造Authorization请求头
	@param [in] token 登录token
	@return 返回请求头映射
	*/
	std::map<std::string, std::string> authHeaders(const std::string& token) const;

	/** 构造账号认证JSON
	@param [in] account 账号
	@param [in] password 密码
	@param [in] nickname 昵称
	@return 返回JSON文本
	*/
	std::string authBody(const std::string& account, const std::string& password, const std::string& nickname) const;

	/** 构造带幂等键的菜谱加入JSON
	@param [in] recipeId 菜谱ID
	@param [in] idempotencyKey 幂等键
	@return 返回JSON文本
	*/
	std::string joinRecipeBody(const std::string& recipeId, const std::string& idempotencyKey) const;

	/** 构造自定义菜谱发布JSON
	@param [in] recipeId 菜谱ID
	@return 返回JSON文本
	*/
	std::string customRecipePublishBody(const std::string& recipeId) const;

	/** 构造个性化配置JSON
	@param [in] recipeId 菜谱ID
	@return 返回JSON文本
	*/
	std::string personalizationBody(const std::string& recipeId) const;

	/** 构造测试用户ID
	@param [in] suffix 用户ID后缀
	@return 返回完整测试用户ID
	*/
	std::string userId(const std::string& suffix) const;

	/** 构造账号查询路径
	@param [in] userId 用户ID
	@return 返回带userId查询参数的接口路径
	*/
	std::string accountQueryPath(const std::string& userId) const;

	/** 构造充值JSON
	@param [in] userId 用户ID
	@param [in] coins 金币数量
	@return 返回JSON文本
	*/
	std::string rechargeBody(const std::string& userId, int32_t coins) const;

	/** 构造菜谱ID JSON
	@param [in] userId 用户ID
	@param [in] recipeId 菜谱ID
	@return 返回JSON文本
	*/
	std::string recipeBody(const std::string& userId, const std::string& recipeId) const;

	/** 构造排程JSON
	@param [in] userId 用户ID
	@param [in] recipeIds 菜谱ID列表
	@return 返回JSON文本
	*/
	std::string planBody(const std::string& userId, const std::vector<std::string>& recipeIds) const;

	/** 写测试报告
	@param [in] allPass 所有测试是否通过
	@return 返回是否写入成功
	*/
	bool writeReport(bool allPass) const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	// 功能测试结果列表
	std::vector<CookServerTestCaseResult> m_caseResults;
	// 压力测试结果写入锁
	std::mutex m_stressMutex;
	// 压力测试汇总
	CookServerStressResult m_stressResult;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};