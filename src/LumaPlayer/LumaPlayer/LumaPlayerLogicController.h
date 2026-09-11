#pragma once
#include "LumaPlayerLogicModels.h"
#include <QObject>
#include <atomic>
#include <map>
#include <deque>
#include <mutex>

/** 每播放器独立逻辑控制器；Qt对象只发送信号，不从后台操作窗口
*/
class LumaPlayerLogicController : public QObject
{
    Q_OBJECT
public:
    /** 初始化安全状态，不创建线程
    @param [in] core 借用桥接对象，外层在全部线程结束后释放
    */
    explicit LumaPlayerLogicController(LumaPlayerCoreBridge* core);

    /** 外层生命周期最后回收协调线程，正常关闭已完成Core收敛
    */
    virtual ~LumaPlayerLogicController();

    /** 创建逻辑及生命周期线程，耗时设备初始化投递后台
    @return 是否成功提交初始化
    */
    bool init();

    /** 提交输入副本，不等待处理
    @param [in] action 输入或结果
    @return true表示接受，false表示关闭或队列已满
    */
    bool submit(const LumaPlayerLogicAction& action);

    /** 只允许逻辑Task调用，串行维护状态
    @param [in] action 本次输入
    */
    void execute(const LumaPlayerLogicAction& action);

    /** 只允许独立生命周期Task调用
    @param [in] shutdown 是否关闭
    @param [in] exitFlag 任务取消标记，初始化安全点观察，关闭不能半途放弃
    */
    void executeLifecycle(bool shutdown, const std::atomic<bool>* exitFlag);

    /** Load业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeLoad(const LumaPlayerLogicAction& action);

    /** View业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeView(const LumaPlayerLogicAction& action);

    /** Reset业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeReset(const LumaPlayerLogicAction& action);

    /** Loop业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeLoop(const LumaPlayerLogicAction& action);

    /** Playback业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executePlayback(const LumaPlayerLogicAction& action);

    /** Window业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeWindow(const LumaPlayerLogicAction& action);

    /** Seek业务Task专用执行入口，不等待Core或GUI
    @param [in] action 逻辑线程持有的输入副本
    */
    void executeSeek(const LumaPlayerLogicAction& action);

signals:
    /** GUI必须用Qt::QueuedConnection接收值副本
    @param [in] result 已确认结果或待应用视图
    */
    void resultReady(const LumaPlayerLogicResult& result);

private:
    /** 结果线程只复制并投递，不直接修改业务状态
    @param [in] completion 回调期借用数据
    @param [in] userData 控制器，生命周期回收完成前有效
    */
    static void coreCompleted(const LumaPlayerCoreCCompletion* completion, void* userData);

    /** 登记身份并提交Core，不等待结果
    @param [in] action 操作参数
    @return 新请求ID，0表示提交失败
    */
    uint64_t submitCore(const LumaPlayerLogicAction& action);

    /** 汇总真实结果并驱动后续短动作
    @param [in] completion 本次结果
    */
    void complete(const LumaPlayerCoreCCompletion& completion);

    /** 按当前状态发送GUI呈现
    @param [in] type 呈现种类
    @param [in] error 错误码
    @param [in] requestId 关联请求，可为0
    @param [in] operation Core操作，可为0
    @param [in] point AB点，可为-1
    @param [in] inputSerial 原始输入序号
    */
    void publish(LumaPlayerActionType type, int32_t error = 0, uint64_t requestId = 0,
        int32_t operation = 0, int32_t point = -1, uint64_t inputSerial = 0);

    /** 暂停完成后投递最新预览或最终定位，不等待解码
    */
    void flushDrag();

    /** 排序拖动尚未完成时收到的播放和AB操作
    @param [in] action 后续操作副本
    @return true表示已接管，调用方不再执行
    */
    bool deferForDrag(const LumaPlayerLogicAction& action);

    /** 仅三个子条件满足且无更新操作时清除重置脏状态
    */
    void finishReset();

private:
    // 外层拥有Core
    LumaPlayerCoreBridge* m_core;
    // 保护提交入口及线程ID
    std::mutex m_submitMutex;
    // 管理器逻辑线程ID
    uint32_t m_logicThreadId;
    // 管理器生命周期线程ID，外层最后回收
    uint32_t m_lifecycleThreadId;
    // 正在关闭，不接受新的业务动作
    std::atomic<bool> m_closing;
    // 初始化是否完成，仅逻辑线程读写
    bool m_ready;
    // 请求编号
    uint64_t m_nextRequestId;
    // 已接受请求参数，真实结果到达后移除
    std::map<uint64_t, LumaPlayerLogicAction> m_pending;
    // 最近的呈现状态，仅逻辑线程修改
    LumaPlayerLogicResult m_state;
    // 初始化前只保留最新加载路径
    QString m_deferredPath;
    // 最近定位请求及累计目标，连续按键不能反复从旧快照起算
    uint64_t m_seekRequest;
    // 定位目标
    int64_t m_seekTarget;
    // 定位前播放意图
    bool m_seekPlaying;
    // 最近倍率请求和累计目标
    uint64_t m_rateRequest;
    // 倍率目标
    int32_t m_rateTarget;
    // 最近播放状态请求
    uint64_t m_playRequest;
    // 请求中的播放意图
    bool m_playTarget;
    // 拖动会话有效，等待暂停或接收预览
    bool m_dragActive;
    // 已收到实际暂停完成
    bool m_dragPaused;
    // 已要求最终定位
    bool m_dragFinishing;
    // 拖动开始输入序号，过滤旧暂停结果
    uint64_t m_dragSerial;
    // 最终定位请求ID
    uint64_t m_dragCommitRequest;
    // 正在执行的唯一预览请求
    uint64_t m_dragPreviewRequest;
    // 最近投递的预览输入序号
    uint64_t m_dragPreviewRevision;
    // 本会话已投递的预览数
    uint64_t m_dragPreviewCount;
    // 最新目标或最终意图
    LumaPlayerLogicAction m_dragLatest;
    // 最终定位后按序执行的业务操作
    std::deque<LumaPlayerLogicAction> m_dragDeferred;
    // 重置关联的倍率请求
    uint64_t m_resetRequest;
    // 重置视图版本
    uint64_t m_resetRevision;
    // GUI已应用位置和缩放
    bool m_resetViewApplied;
    // Core倍率实际达到默认值
    bool m_resetRateApplied;
};