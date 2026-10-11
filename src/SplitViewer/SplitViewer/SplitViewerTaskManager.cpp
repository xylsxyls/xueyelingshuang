#include "SplitViewerTaskManager.h"
#include "SplitViewerTaskLifecycleManager.h"
#include "SplitViewerLogicTask.h"
#include "Config.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <exception>

SplitViewerTaskManager::SplitViewerTaskManager(QObject* parent) :
QObject(parent),
m_context(std::make_shared<SplitViewerTaskContext>()),
m_timer(new QTimer(this)),
m_busy(false),
m_stopping(false),
m_nextId(0)
{
    m_timer->setInterval(g_config.m_taskPollIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &SplitViewerTaskManager::poll);
    m_timer->start();
}

SplitViewerTaskManager::~SplitViewerTaskManager()
{
    stop();
}

bool SplitViewerTaskManager::init()
{
    if (m_stopping || !SplitViewerTaskLifecycleManager::instance().init())
    {
        return false;
    }
    try
    {
        if (!m_context->m_logicId)
        {
            m_context->m_logicId = CTaskThreadManager::Instance().Init();
        }
        if (!m_context->m_executionId)
        {
            m_context->m_executionId = CTaskThreadManager::Instance().Init();
        }
        if (!m_context->m_logicId || !m_context->m_executionId)
        {
            stop();
            return false;
        }
        return true;
    }
    catch (const std::exception& error)
    {
        LOGERROR("Window task initialization failed; error=%s", error.what());
        stop();
        return false;
    }
}

bool SplitViewerTaskManager::submit(bool longRunning, const std::function<bool(const std::atomic<bool>&)>& work,
    const std::function<void(bool)>& complete)
{
    if (m_stopping || m_busy || !work || !complete || !m_context->m_logicId || !m_context->m_executionId)
    {
        return false;
    }
    const std::shared_ptr<CTaskThread> logic = CTaskThreadManager::Instance().GetThreadInterface(m_context->m_logicId);
    if (!logic)
    {
        return false;
    }
    try
    {
        const std::shared_ptr<SplitViewerTaskRequest> request = std::make_shared<SplitViewerTaskRequest>();
        request->m_id = ++m_nextId;
        request->m_longRunning = longRunning;
        request->m_work = work;
        request->m_complete = complete;
        logic->PostTask(std::make_shared<SplitViewerLogicTask>(m_context, request));
        m_busy = true;
        LOGINFO("Request accepted; request=%llu logic=%u execution=%u", request->m_id, m_context->m_logicId,
            m_context->m_executionId);
    }
    catch (const std::exception& error)
    {
        m_busy = false;
        LOGERROR("Request admission failed; error=%s", error.what());
        return false;
    }
    return true;
}

void SplitViewerTaskManager::stop()
{
    if (!m_stopping)
    {
        m_stopping = true;
        SplitViewerTaskLifecycleManager::instance().retire(m_context);
    }
}

bool SplitViewerTaskManager::busy() const
{
    return m_stopping ? !m_context->m_retired.load() : m_busy;
}

void SplitViewerTaskManager::poll()
{
    const std::shared_ptr<SplitViewerTaskRequest> request =
        std::atomic_exchange(&m_context->m_completed, std::shared_ptr<SplitViewerTaskRequest>());
    if (request)
    {
        m_busy = false;
        LOGINFO("Request completed; request=%llu success=%d", request->m_id, request->m_success);
        if (!m_stopping)
        {
            request->m_complete(request->m_success);
        }
    }
}