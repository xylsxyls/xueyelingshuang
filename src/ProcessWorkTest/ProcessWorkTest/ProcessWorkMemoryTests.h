#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include <atomic>

/** 记录策略调用和成功事实，支持拒绝及抛异常反例；只编入Test工程
*/
class ProcessWorkTestMemoryPolicy : public ProcessWorkMemoryPolicy
{
public:
    // 策略调用数，不等同成功次数
    std::atomic<uint64_t> m_calls;
    // 成功通知数
    std::atomic<uint64_t> m_notifications;
    // 所有成功事件的增量和
    std::atomic<uint64_t> m_added;
    // 按类型累计真实成功次数及增量，仅含实际分配
    std::atomic<uint64_t> m_kindCount[3];
    // 每种类型的成功增量合计
    std::atomic<uint64_t> m_kindAdded[3];
    // 非零时返回固定增量，零使用基类
    uint64_t m_increment;
    // 拒绝增长
    bool m_refuse;
    // 验证策略异常处理
    bool m_throwGrow;
    // 验证通知异常不回滚
    bool m_throwNotification;

public:
    /** 初始化观察
    */
    ProcessWorkTestMemoryPolicy();

    /** 返回增量或触发明确反例
    @param [in] context 库给出的真实上下文
    @return 建议增量
    */
    virtual uint64_t grow(const ProcessWorkMemoryGrowContext& context);

    /** 累计实际成功增量
    @param [in] growth 已完成增长
    */
    virtual void grown(const ProcessWorkMemoryGrowth& growth);
};

/** 在配置复制时抛异常的测试观察者，只用于验证错误码和原配置保留
*/
class ProcessWorkTestGrowthObserver
{
public:
    /** 借用故障开关，测试对象存活期间必须有效
    @param [in] fail 是否让复制抛异常
    */
    explicit ProcessWorkTestGrowthObserver(bool& fail);

    /** 根据故障开关复制
    @param [in] other 原观察者
    */
    ProcessWorkTestGrowthObserver(const ProcessWorkTestGrowthObserver& other);

    /** 空观察，不改变增长事实
    @param [in] added 实际增量
    @param [in] sequence 成功次数观察值
    */
    void operator()(size_t added, uint64_t sequence) const;

private:
    // 测试入口持有，静止期修改
    bool* m_fail;
};

/** 固定额度精确边界、无上限跳过字节计费、生命周期保护和永久超额拒绝
*/
void testProcessWorkFixedBudget();

/** 独立发送进程核对接收端公布的单项额度，超额拒绝后精确边界仍可成功
@param [in] pid 额度300字节且正在接收的父进程
@return 成功为0，失败通过测试框架记录
*/
int32_t runProcessWorkQuotaProbe(int32_t pid);

/** 核对默认-1、固定本地额度、正文通知以及初始化参数真正进入内部组件
*/
void testProcessWorkMemoryConfiguration();

/** 三个基础容器初始预留不通知，真实增长通知与统计一致，通知异常不撤销提交
*/
void testProcessWorkNativeGrowth();

/** Post动态缓存、有限裁剪、并发增长及所有发送者进展
*/
void testProcessWorkPacketGrowth();

/** 共享块增长策略裁剪与实际分配通知，通过真实通道发送并核对完整正文
*/
void testProcessWorkSharedGrowth();