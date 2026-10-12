#pragma once
#include "CookServerTestFeedCases.h"
#include "RapidJson/RapidJsonAPI.h"

/** 付费作品审核、额度、持久结果及资源迁移的真实回归，追加78至86。 */
class CookServerTestReviewCases
{
  public:
    /** 顺序登记本模块用例。
    @param [in] client 本批隔离HTTP客户端
    @param [in] registerCase 稳定用例登记入口
    */
    static void run(CookServerTestHttpClient &client, const CookServerTestFeedCases::RegisterFunction &registerCase);

  private:
    /** 请求实际接口，不在客户端模拟审核结果。
    @param [in] client HTTP客户端
    @param [in] token 已认证会话
    @param [in] route 接口路径
    @param [in] body 空表示GET
    @param [out] result 响应JSON
    @param [out] message 失败证据
    @param [in] expectedOk 预期业务成功值
    @return HTTP及业务符合预期返回true
    */
    static bool call(CookServerTestHttpClient &client, const std::string &token, const std::string &route,
        const std::string &body, RapidJsonDocument &result, std::string &message, bool expectedOk = true);
    /** 生成独立作者、首版收费投稿及管理员会话。
    @param [in] client 客户端
    @param [in] name 用例标识
    @param [out] token 作者会话
    @param [out] admin 管理员会话
    @param [out] recipe 实际服务端菜谱
    @param [out] message 失败原因
    @param [in] price 初始收费方式
    @return 准备成功返回true
    */
    static bool fixture(CookServerTestHttpClient &client, const std::string &name, std::string &token,
        std::string &admin, RapidJsonValue &recipe, std::string &message, const std::string &price = "coin");
    /** 构造固定独立原始菜谱。
    @param [in] id 客户端防重复标识
    @param [in] price 定价方式
    @return 实际发布请求
    */
    static std::string publication(const std::string &id, const std::string &price);
    /** 以已读取版本构造价格修改，保留所有实际步骤。
    @param [in] recipe 实际菜谱
    @param [in] amount 目标金额
    @return 编辑请求
    */
    static std::string edit(const RapidJsonValue &recipe, int32_t amount);
    /** 构造管理员决定请求。
    @param [in] recipe 实际投稿
    @param [in] decision 决定
    @param [in] reason 说明
    @return 审核请求
    */
    static std::string decision(
        const RapidJsonValue &recipe, const std::string &decision, const std::string &reason = "");
    /** 检查HTTP场景的业务判定。
    @param [in] condition 实际条件
    @param [in] detail 条件未满足的原因
    @param [out] message 证据
    @return 条件值
    */
    static bool check(bool condition, const std::string &detail, std::string &message);
};