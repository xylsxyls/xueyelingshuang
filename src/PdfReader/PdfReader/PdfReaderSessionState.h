#pragma once
#include "PdfReaderResult.h"
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>

class QObject;

/** 跨线程会话；Core仅执行者访问，结果邮箱与接收者由同一锁保护
*/
class PdfReaderSessionState : public std::enable_shared_from_this<PdfReaderSessionState>
{
public:
    /** GUI中复制配置，不初始化PDF资源
    @param [in] receiver 仅用来QueuedConnection投递完成通知
    */
    explicit PdfReaderSessionState(QObject* receiver);

    /** 在逻辑线程分派请求；失败有终态
    @param [in] request 已分配身份的请求
    */
    void dispatch(const PdfReaderRequest& request);

    /** 在唯一执行者操作Core，不访问QWidget
    @param [in] result 当前请求结果
    @param [in] exit 协作取消标志
    */
    void execute(const std::shared_ptr<PdfReaderResult>& result, const std::atomic<bool>& exit);

    /** 逻辑层检查代次后发布结果，调用不持业务锁
    @param [in] result 已完成的请求
    */
    void deliver(const std::shared_ptr<PdfReaderResult>& result);

    /** 工作异常/投递异常的失败出口；仍回报请求身份
    @param [in] result 请求结果
    @param [in] error 异常说明
    */
    void fail(const std::shared_ptr<PdfReaderResult>& result, const std::string& error);
public:
    // 进程内唯一会话身份，构造后不变
    const uint64_t m_sessionId;
    // GUI递增；执行/逻辑只读，隔离过期预览
    std::atomic<uint64_t> m_epoch;
    // GUI关闭即置位，工作中的一次PDFium调用允许自然结束
    std::atomic<bool> m_closing;
    // 以下接收者及邮箱只在此锁内访问，不在锁内执行用户回调
    std::mutex m_mutex;
    QObject* m_receiver;
    std::deque<std::shared_ptr<PdfReaderResult>> m_results;
private:
    // GUI创建空桥接；句柄创建、调用与最终释放全部在执行者
    std::unique_ptr<PdfReaderCoreBridge> m_core;
    // GUI启动时的参数快照；任务不读取可变g_config
    const PdfReaderCoreCConfig m_config;
    bool m_initialized;
};