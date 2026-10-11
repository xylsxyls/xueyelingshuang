#pragma once
#include "CookServerTestHttpClient.h"
#include <functional>
#include <vector>

/** 真实HTTP个人作品与收藏合集回归，追加51至53号用例。
*/
class CookServerTestFeedCases
{
public:
    typedef std::function<bool(std::string&, int32_t&)> CaseFunction;
    typedef std::function<void(const std::string&, const CaseFunction&)> RegisterFunction;
    /** 执行个人媒体筛选回归，所有写入仅使用隔离测试账号。
    @param [in] client HTTP客户端
    @param [in] token 已认证作品作者会话
    @param [in] createdVideoId 此作者真实上传并发布的作品
    @param [in] registerCase 顺序执行并记录测试结果
    */
    static void run(CookServerTestHttpClient& client, const std::string& token,
                    const std::string& createdVideoId, const RegisterFunction& registerCase);
private:
    /** 检查网络、状态码和JSON业务结果。
    @param [in] response 实际响应
    @param [in] expectedStatus 预期状态
    @param [in] expectedOk 预期业务结果
    @param [out] message 失败原因
    @param [out] statusCode 实际状态
    @return 返回是否满足预期
    */
    static bool check(const CookServerTestHttpResponse& response, int32_t expectedStatus,
                      bool expectedOk, std::string& message, int32_t& statusCode);
    /** 检查完整个人视频列表、唯一性及看过状态不重置。
    @param [in] client HTTP客户端
    @param [in] token 查询会话
    @param [in] query 模式、排除和分页查询参数
    @param [in] expectedIds 完整预期视频ID集合
    @param [in] owner 非空时要求全部作品属于此作者
    @param [out] message 失败原因
    @param [out] statusCode 实际状态
    @return 返回是否满足预期
    */
    static bool expectFeed(CookServerTestHttpClient& client, const std::string& token,
                           const std::string& query, const std::vector<std::string>& expectedIds,
                           const std::string& owner, std::string& message, int32_t& statusCode);
    /** 切换并断言收藏状态，避免未清理的测试状态造成假通过。
    @param [in] client HTTP客户端
    @param [in] token 收藏账号会话
    @param [in] type 收藏目标类型
    @param [in] id 收藏目标ID
    @param [in] expected 收藏切换后的预期状态
    @param [out] message 失败原因
    @param [out] statusCode 实际状态
    @return 返回是否满足预期
    */
    static bool toggle(CookServerTestHttpClient& client, const std::string& token,
                       const std::string& type, const std::string& id, bool expected,
                       std::string& message, int32_t& statusCode);
};