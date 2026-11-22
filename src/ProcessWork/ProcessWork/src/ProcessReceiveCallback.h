#pragma once
#include <stdint.h>
#include "ProcessWorkMacro.h"

/** 借用的接收观察者，由调用方保活到接收任务完全退出；库不经此基类销毁对象
*/
class ProcessWorkAPI ProcessReceiveCallback
{
public:
    /** 在单一回调线程依次接收；正文仅在本次回调期间有效，异常由分派边界隔离
    @param [in] sendPid 发送进程ID
    @param [in] buffer 借用正文，空包为nullptr；需要异步保存时应在返回前复制
    @param [in] length 数据长度
    @param [in] type 数据类型
    */
    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type) = 0;

    /** 缓冲区满事件，来自独立接收回调任务；同一轮满载合并通知
    @param [in] count 观察满载时当前活动块数快照，不是永久容量或实时承诺
    回调不可等待自身销毁或依赖本回调释放的容量；回投本进程宜使用默认不重试的try接口并处理失败
    默认不处理，业务实现不应向分派方抛异常
    */
    virtual void bufferFull(int32_t count)
    {
        (void)count;
    }
};