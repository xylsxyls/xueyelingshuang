#pragma once
#include "PlayerCommand.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class PlayerEngine;

/** 单条控制任务，在工作线程串行修改引擎状态
*/
class PlayerCommandTask : public CTask
{
public:
    // 防止外部命令洪泛占用无限内存，达到上限时明确拒绝新提交
    static const int32_t kMaxPendingCommands = 4096;
    // 控制任务类别ID起点，避开播放与预览任务ID
    static const int32_t kTaskIdBase = 100;
    // 控制优先级高于后台播放
    static const int32_t kPriority = 2;

    /** 复制命令数据并初始化取消标记
    @param [in] engine 借用的引擎，必须存活到线程退出
    @param [in] command 命令及共享完成通知
    */
    PlayerCommandTask(PlayerEngine* engine, const PlayerCommand& command);

    /** 已取消时只通知结果，否则执行控制命令
    */
    virtual void DoTask();

    /** 设置协作退出标记，不阻塞调用线程
    */
    virtual void StopTask();

private:
    // 引擎指针，不负责释放
    PlayerEngine* m_engine;
    // 命令副本
    PlayerCommand m_command;
    // 协作退出标记
    std::atomic<bool> m_exit;
};