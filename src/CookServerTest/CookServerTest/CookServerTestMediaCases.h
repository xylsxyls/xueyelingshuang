#pragma once
#include "CookServerTestHttpClient.h"
#include <functional>

/** 真实HTTP媒体边界测试，历史用例之后追加44至50。
*/
class CookServerTestMediaCases
{
public:
    typedef std::function<bool(std::string&, int32_t&)> CaseFunction;
    typedef std::function<void(const std::string&, const CaseFunction&)> RegisterFunction;
    /** 执行媒体测试；仅使用指定隔离账号会话和已发布素材。
    @param [in] client HTTP客户端
    @param [in] token 已认证测试会话
    @param [in] videoId 已发布测试视频
    @param [in] registerCase 按顺序执行并记录用例
    */
    static void run(CookServerTestHttpClient& client, const std::string& token,
                    const std::string& videoId, const RegisterFunction& registerCase);
private:
    /** 断言网络、状态码和JSON业务结果。
    @param [in] response 实际HTTP响应
    @param [in] status 预期HTTP状态
    @param [in] ok 预期业务结果
    @param [out] message 失败原因
    @param [out] statusCode 实际状态
    @return 返回是否符合预期
    */
    static bool check(const CookServerTestHttpResponse& response, int32_t status, bool ok,
                      std::string& message, int32_t& statusCode);
    /** 创建4字节上传任务并读取真实会话ID。
    @param [in] client HTTP客户端
    @param [out] message 失败原因
    @param [out] statusCode 实际状态
    @return 返回任务ID；失败为空
    */
    static std::string start(CookServerTestHttpClient& client, std::string& message, int32_t& statusCode);
    /** 清理本用例上传会话。
    @param [in] client HTTP客户端
    @param [in] id 任务ID
    */
    static void cancel(CookServerTestHttpClient& client, const std::string& id);
};