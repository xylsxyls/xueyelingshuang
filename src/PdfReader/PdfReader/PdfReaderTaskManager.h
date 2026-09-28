#pragma once
#include "PdfReaderTask.h"
#include <stdint.h>

/** 进程内唯一调度器；所有 PDFium 会话共用一个执行者，GUI 不等待线程
*/
class PdfReaderTaskManager
{
public:
    /** 按需取得空管理对象；不在静态初始化创建线程
    @return 进程调度器
    */
    static PdfReaderTaskManager& instance();

    /** main 正常启动阶段调用；部分创建失败由最终清理收回
    */
    void init();

    /** 所有窗口销毁、GUI事件循环结束后调用，收回本对象的两个ID
    */
    void finish();

    /** 将短逻辑任务送入逻辑线程
    @param [in] task 有限任务
    */
    void postLogic(const std::shared_ptr<CTask>& task);

    /** 仅逻辑任务分派核心操作到唯一PDF执行者
    @param [in] task 有限任务
    */
    void postWork(const std::shared_ptr<CTask>& task);
    /** 分配日志和回报使用的进程内会话身份
    @return 非零递增会话序号
    */
    uint64_t nextSessionId();

private:
    /** 初始化为空；线程由init显式建立
    */
    PdfReaderTaskManager();

    /** 禁止复制线程归属
    @param [in] other 不使用
    */
    PdfReaderTaskManager(const PdfReaderTaskManager& other);

    /** 禁止转移线程归属
    @param [in] other 不使用
    @return 不实现
    */
    PdfReaderTaskManager& operator=(const PdfReaderTaskManager& other);
private:
    // ID创建后只读；finish在所有GUI生产者销毁后调用
    // 仅分配身份，不保存任何会话所有权
    std::atomic<uint64_t> m_nextSessionId;
    uint32_t m_logic;
    uint32_t m_worker;
};