#include "PdfReaderTask.h"
#include <exception>

PdfReaderTask::PdfReaderTask(const std::function<void(const std::atomic<bool>&)>& action,
    const std::function<void(const std::string&)>& failure) :
m_exit(false), m_action(action), m_failure(failure)
{

}

void PdfReaderTask::DoTask()
{
    try
    {
        m_action(m_exit);
    }
    catch (const std::exception& error)
    {
        m_failure(error.what());
    }
    catch (...)
    {
        m_failure("unknown task exception");
    }
}

void PdfReaderTask::StopTask()
{
    m_exit.store(true);
}