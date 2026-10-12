#pragma once
#include "CookServerTestFeedCases.h"
#include "RapidJson/RapidJsonAPI.h"

/** 作者菜谱编辑的真实HTTP回归，追加70至77号用例 */
class CookServerTestRecipeEditingCases
{
public:
    /** 按稳定顺序登记作者保存、拒绝边界及系统菜谱编辑
    @param [in] client 本批隔离HTTP客户端
    @param [in] registerCase 运行并记录结果的入口
    */
    static void run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);

private:
    /** 登记结构编辑、拒绝边界及原个性化兼容回归
    @param [in] client 隔离HTTP客户端
    @param [in] registerCase 运行并记录结果的入口
    */
    static void runStructureCases(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);

    /** 从真实菜谱构造完整结构编辑请求
    @param [in] recipe 原始菜谱及版本
    @return 包含editorVersion=2与真实步骤性质的请求
    */
    static RapidJsonDocument structureBody(const RapidJsonValue& recipe);

    /** 登记定价边界及系统菜谱收费权益回归
    @param [in] client 隔离HTTP客户端
    @param [in] registerCase 运行并记录结果的入口
    */
    static void runPricingCases(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);

    /** 发出真实HTTP请求并按独立业务预期判定
    @param [in] client 客户端
    @param [in] token 会话，可为空
    @param [in] path API相对路径
    @param [in] body 空为GET，否则POST
    @param [out] json 实际响应
    @param [out] message 失败说明
    @param [in] expectedOk 预期业务状态
    @return 网络、解析及业务结果一致时返回true
    */
    static bool call(CookServerTestHttpClient& client, const std::string& token, const std::string& path,
        const std::string& body, RapidJsonDocument& json, std::string& message, bool expectedOk = true);

    /** 注册作者并发布两步固定测试菜谱
    @param [in] client 客户端
    @param [in] suffix 本项隔离标识
    @param [out] user 注册账号及会话
    @param [out] recipe 本人原始菜谱
    @param [out] message 失败说明
    @return 准备完成时返回true
    */
    static bool fixture(CookServerTestHttpClient& client, const std::string& suffix,
        RapidJsonDocument& user, RapidJsonValue& recipe, std::string& message);

    /** 从原始菜谱生成步骤编辑请求，保留原定价并伪造受保护字段以核验隔离
    @param [in] recipe 原始菜谱及版本
    @param [in] title 首步骤的新标题
    @param [in] seconds 首步骤的新时长
    @return 保存请求的JSON文档
    */
    static RapidJsonDocument body(const RapidJsonValue& recipe, const std::string& title, int32_t seconds);
};