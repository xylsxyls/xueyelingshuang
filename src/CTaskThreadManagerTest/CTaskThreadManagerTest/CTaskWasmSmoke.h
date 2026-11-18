#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <vector>

/** Wasm 消费者测试任务；借用期覆盖管理器排空，生产库无测试钩子
*/
class CTaskWasmSmokeTask : public CTask
{
public:
    /** 构造观察任务，gate 非空时阻挡线程以建立确定的待执行优先级集合
    @param [in] value 写入账本的任务编号
    @param [in,out] ledger 仅工作线程写，join 完成后主线程读取
    @param [in,out] gate 可空，非空时等待主线程 release 发布
    */
    CTaskWasmSmokeTask(int32_t value, std::vector<int32_t>& ledger, std::atomic<bool>* gate);

    /** 等待可选门并记录一次执行
    */
    void doTask() override;

private:
    // 不变的任务编号
    int32_t m_value;
    // 借用消费者私有账本
    std::vector<int32_t>* m_ledger;
    // 借用可选测试门
    std::atomic<bool>* m_gate;
};