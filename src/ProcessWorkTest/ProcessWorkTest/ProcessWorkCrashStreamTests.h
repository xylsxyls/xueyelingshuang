#pragma once
#include "ProcessWorkTest.h"
#include <map>

/** 强制终止测试拥有的进程；不得用于生产库或不属于当前用例的进程
@param [in,out] child 本用例创建的子进程
*/
void CrashOwnedChild(BlackBoxChild& child);

/** 普通send持续运行的子进程，只由父用例在发送过程中终止
@param [in] argc 参数数量
@param [in] argv 参数列表
@return 子进程状态
*/
int runProcessWorkCrashStream(int argc, char** argv);

/** 六次实际崩溃与存活发送者交错；验证逐流前缀、存活流无损和所有占位回收
*/
void testProcessWorkCrashStream();

/** 唯一回调线程校验每个PID/流的连续前缀，向主线程发布计数
*/
class ProcessWorkCrashLedger : public ProcessReceiveCallback
{
public:
    /** 初始化存活发送者和崩溃发送者的独立计数
    */
    ProcessWorkCrashLedger();

    /** 核对六字节正文与每条流的连续前缀，分别累计本进程和崩溃进程消息
    @param [in] pid 实际发送进程
    @param [in] data 本次回调内借用的正文，空正文为nullptr
    @param [in] length 正文字节数
    @param [in] type 发送端附带的测试序号
    */
    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

public:
    // 存活的本进程发送者交付数
    std::atomic<int32_t> m_ownCount;
    // 被终止子进程的已交付前缀总数
    std::atomic<int32_t> m_childCount;
    // 逐流校验错误，必须在接收线程结束后仍为0
    std::atomic<int32_t> m_errors;

private:
    // 只有接收回调线程访问，每个PID及线程单独校验连续序号。
    std::map<std::pair<int32_t, int32_t>, int32_t> m_next;
};