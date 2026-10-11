#pragma once
#include "CookServerTestFeedCases.h"
#include "RapidJson/RapidJsonAPI.h"

/** 个人资料、流程插入和多账号视频状态的真实HTTP回归。 */
class CookServerTestUiRefineCases
{
public:
    /** 注册63至66号用例。
    @param [in] client 测试HTTP客户端
    @param [in] registerCase 记录每项执行结果
    */
    static void Run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);
private:
    /** 请求并断言业务结果。
    @param [in] client 客户端 @param [in] token 会话 @param [in] path 路径
    @param [in] body 请求体，空为GET @param [in] ok 预期业务状态 @return JSON结果
    */
    static RapidJsonDocument Request(CookServerTestHttpClient& client, const std::string& token,
        const std::string& path, const std::string& body = "", bool ok = true);
    /** 创建隔离账号。@param [in] client 客户端 @param [in] suffix 用例标识 @return 注册结果 */
    static RapidJsonDocument User(CookServerTestHttpClient& client, const std::string& suffix);
    /** 组装带完整顺序的配置。
    @param [in] order 步骤顺序 @param [in] added 新增步骤JSON @param [in] disabled 禁用步骤
    @return 保存请求体
    */
    static std::string Config(const std::vector<std::string>& order, const std::string& added,
        const std::vector<std::string>& disabled = std::vector<std::string>());
    /** 断言条件。@param [in] value 条件 @param [in] message 失败说明 */
    static void Require(bool value, const char* message);
};