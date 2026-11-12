#pragma once
#include "ProcessWorkTest.h"
#include <map>

/** 强制终止测试拥有的进程；不得用于生产库或不属于当前用例的进程。
@param [in,out] child 本用例创建的子进程
*/
void CrashOwnedChild(BlackBoxChild& child);

/** 普通send持续运行的子进程，只由父用例在发送过程中终止。
@param [in] argc 参数数量
@param [in] argv 参数列表
@return 子进程状态
*/
int runProcessWorkCrashStream(int argc, char** argv);

/** 六次实际崩溃与存活发送者交错；验证逐流前缀、存活流无损和所有占位回收。
*/
void testProcessWorkCrashStream();

class ProcessWorkCrashLedger : public ProcessReceiveCallback
{
public:
    ProcessWorkCrashLedger();

    virtual void receive(int32_t pid, const char* data, int32_t length, MessageType type);

public:
    std::atomic<int32_t> ownCount;
    std::atomic<int32_t> childCount;
    std::atomic<int32_t> errors;

private:
    // 只有接收回调线程访问，每个PID及线程单独校验连续序号。
    std::map<std::pair<int32_t, int32_t>, int32_t> m_next;
};