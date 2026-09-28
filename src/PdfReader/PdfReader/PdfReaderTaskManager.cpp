#include "PdfReaderTaskManager.h"
#include "LogManager/LogManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <stdexcept>

PdfReaderTaskManager::PdfReaderTaskManager() : m_nextSessionId(0), m_logic(0), m_worker(0)
{

}

PdfReaderTaskManager& PdfReaderTaskManager::instance()
{
    static PdfReaderTaskManager s_runtime;
    return s_runtime;
}

void PdfReaderTaskManager::init()
{
    CTaskThreadManager& manager = CTaskThreadManager::Instance();
    if (!m_logic)
    {
        m_logic = manager.Init();
    }
    if (!m_worker)
    {
        m_worker = manager.Init();
    }
    if (!m_logic || !m_worker)
    {
        throw std::runtime_error("PdfReader task thread creation failed");
    }
    LOGINFO("PdfReader threads logic=%u worker=%u", m_logic, m_worker);
}

void PdfReaderTaskManager::postLogic(const std::shared_ptr<CTask>& task)
{
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_logic);
    if (!thread)
    {
        throw std::runtime_error("PdfReader logic thread unavailable");
    }
    thread->PostTask(task, 1);
}

void PdfReaderTaskManager::postWork(const std::shared_ptr<CTask>& task)
{
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(m_worker);
    if (!thread)
    {
        throw std::runtime_error("PdfReader worker thread unavailable");
    }
    thread->PostTask(task, 1);
}

void PdfReaderTaskManager::finish()
{
    // This final application phase runs after every GUI producer has been destroyed.
    // The fence drains dispatch before stopping the worker; completions may still
    // enter the logic queue until that worker has exited. No GUI callback is needed.
    CTaskThreadManager& manager = CTaskThreadManager::Instance();
    if (m_logic)
    {
        std::shared_ptr<Semaphore> fence(new Semaphore());
        postLogic(std::shared_ptr<CTask>(new PdfReaderTask(
            [fence](const std::atomic<bool>&) { fence->signal(); },
            [fence](const std::string&) { fence->signal(); })));
        fence->wait();
    }
    if (m_worker)
    {
        LOGINFO("PdfReader final worker drain begin id=%u", m_worker);
        manager.WaitForEnd(m_worker);
        m_worker = 0;
    }
    if (m_logic)
    {
        manager.WaitForEnd(m_logic);
        m_logic = 0;
    }
    LOGINFO("PdfReader final thread drain complete");
}

uint64_t PdfReaderTaskManager::nextSessionId()
{
    return ++m_nextSessionId;
}