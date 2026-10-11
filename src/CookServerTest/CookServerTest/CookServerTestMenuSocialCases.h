#pragma once
#include "CookServerTestFeedCases.h"
#include "RapidJson/RapidJsonAPI.h"

/** 菜单、公开资料及私信边界，全部使用独立隔离账号。 */
class CookServerTestMenuSocialCases
{
public:
    /** 注册54至61号用例。@param [in] client HTTP客户端 @param [in] fixtureVideoId 已发布真实视频
    @param [in] registerCase 记录执行结果 */
    static void Run(CookServerTestHttpClient& client, const std::string& fixtureVideoId,
        const CookServerTestFeedCases::RegisterFunction& registerCase);
private:
    /** 执行并校验真实JSON响应。@param [in] client 客户端 @param [in] token 会话 @param [in] path 路径
    @param [in] body 空表示GET @param [in] expectedOk 预期业务状态 @param [in] status 预期HTTP状态 @return 响应JSON */
    static RapidJsonDocument Request(CookServerTestHttpClient& client, const std::string& token,
        const std::string& path, const std::string& body = "", bool expectedOk = true, int32_t status = 200);
    /** 创建用例专用账号。@param [in] client 客户端 @param [in] suffix 用例标识 @return 注册响应 */
    static RapidJsonDocument NewUser(CookServerTestHttpClient& client, const std::string& suffix);
    /** 提交一个简短合法免费菜谱。@param [in] client 客户端 @param [in] token 作者会话 @param [in] id 菜谱
    @param [in] title 步骤名 @param [in] grams 用量 @param [in] expectedOk 预期 @return 响应 */
    static RapidJsonDocument Recipe(CookServerTestHttpClient& client, const std::string& token, const std::string& id,
        const std::string& title = "Cut", const std::string& grams = "100\\u514b", bool expectedOk = true);
    /** 抛出明确断言。@param [in] valid 条件 @param [in] message 错误说明 */
    static void Require(bool valid, const char* message);
};