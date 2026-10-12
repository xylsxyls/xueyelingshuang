#pragma once
#include <stdint.h>
#include <string>

class RapidJsonValue;

/** 不可变投稿内容的差异记录及唯一审核决定，状态由AccountStore锁内维护 */
struct CookReviewRevision
{
  public:
    // 服务端生成的唯一投稿版本
    std::string m_id;
    // 稳定菜谱标识
    std::string m_recipeId;
    // 投稿作者
    std::string m_ownerUserId;
    // 用于差异重建的上一投稿版本，空表示基线
    std::string m_parentId;
    // 提交时最近通过版本，baseline表示升级前有效版本，空表示无通过版本
    std::string m_previousApprovedId;
    // 仅记录相对上一投稿的实际变化
    std::string m_deltaJson;
    // pending、approved、rejected、superseded
    std::string m_state;
    // 审核员身份，免费自动发布为空
    std::string m_reviewerUserId;
    // 不通过说明
    std::string m_reason;
    // 本菜累计第几次提交，包含免费自动发布
    int32_t m_submissionNumber;
    // 本菜第几次付费审核，免费为0
    int32_t m_reviewNumber;
    // 服务端接收提交时间，Unix秒
    int32_t m_submittedAtSeconds;
    // 实际完成审核时间，Unix秒；未处理为0
    int32_t m_reviewedAtSeconds;
    // 同秒审核结果的稳定排序序列
    int32_t m_decisionSequence;
    // 是否要求人工审核
    bool m_requiresReview;

    /** 初始化尚未提交的安全状态 */
    CookReviewRevision();

    /** 序列化元数据与差异，不复制整份菜谱
    @param [in] withDelta 持久化时带差异，列表及历史摘要不带大段差异文本
    @return 持久/响应JSON对象
    */
    RapidJsonValue toJson(bool withDelta = false) const;

    /** 读取持久记录并验证状态、身份和数字范围
    @param [in] json 持久JSON
    @return 记录完整有效返回true
    */
    bool loadJson(const std::string &json);
};