#pragma once
#include <atomic>

/** 仅候选携带复制观察量，已发布值不保留测试状态
*/
struct LockFreeMapRetryValue
{
public:
    // 业务载荷
    int m_value;
    // 候选复制次数，寿命覆盖整个调用
    std::atomic<unsigned>* m_copies;

public:
    /** 初始化候选或普通值
    @param [in] value 载荷
    @param [in,out] copies 可空复制观察量
    */
    explicit LockFreeMapRetryValue(int value = 0, std::atomic<unsigned>* copies = nullptr);

    /** 计数并复制载荷，存储值不携带观察量
    @param [in] other 源候选
    */
    LockFreeMapRetryValue(const LockFreeMapRetryValue& other);

    /** 复制载荷，清除观察量
    @param [in] other 源值
    @return 当前对象
    */
    LockFreeMapRetryValue& operator=(const LockFreeMapRetryValue& other);
};

/** 检查竞争、容量开关、策略拒绝及重入资源错误
@return 全部独立预期满足时true
*/
bool LockFreeMapRetryTest();

/** 检查两池通知时机、增量和异常隔离
@return 全部独立预期满足时true
*/
bool LockFreeMapGrowthTest();