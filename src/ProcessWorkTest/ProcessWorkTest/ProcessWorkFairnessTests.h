#pragma once
#include "ProcessWorkTest.h"

/** 公平性压力账本；每流独立验证序号和全部正文，只保留计数，不缓存消息实体
*/
class ProcessWorkFairnessReceiver : public ProcessReceiveCallback
{
public:
    /** 在启动接收器前构造固定正文及逐流账本
    @param [in] mixed true交替使用六字节和64KiB正文，false全部六字节
    */
    explicit ProcessWorkFairnessReceiver(bool mixed);

    /** 验证真实回调的来源、顺序和正文，并发布该流累计完成数
    @param [in] pid 发送进程ID
    @param [in] data 借用正文
    @param [in] length 正文字节数
    @param [in] type 高五位流编号，低26位流内序号
    */
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

public:
    // 准备完成后只读的每流测试正文
    std::string m_bodies[32];
    // 回调发布的逐流完成数，主测试线程读取
    std::atomic<int32_t> m_received[32];
    // 校验错误计数
    std::atomic<int32_t> m_errors;

private:
    // 唯一回调线程维护的下一序号
    int32_t m_next[32];
};

/** 持续竞争中逐窗口检查每个活跃发送者和接收流都推进；不是仅检查最终总数
@param [in] mixed true大小包、晚加入及暂停恢复，false同尺寸32线程争用单槽
*/
void testProcessWorkFairProgress(bool mixed);