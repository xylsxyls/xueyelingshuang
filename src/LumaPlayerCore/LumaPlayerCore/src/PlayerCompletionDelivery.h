#pragma once
#include "LumaPlayerCoreRequest.h"
#include <atomic>

/** 提交前分配的结果信封，完成或取消时不再分配内存
*/
struct PlayerCompletionDelivery
{
public:
    // 唯一执行者写入，ready发布后结果线程读取
    LumaPlayerCoreCompletion m_completion;
    // 宿主回调，结果线程调用
    LumaPlayerCoreCompletionCallback m_callback;
    // true表示请求实际接受；拒绝的信封只回收不回调
    bool m_accepted;
    // 结果发布屏障
    std::atomic<bool> m_ready;

public:
    /** 复制参数和回调，所有可能失败的分配发生在接受请求之前
    @param [in] request 请求参数
    @param [in] callback 宿主回调
    */
    PlayerCompletionDelivery(const LumaPlayerCoreRequest& request, const LumaPlayerCoreCompletionCallback& callback);
};