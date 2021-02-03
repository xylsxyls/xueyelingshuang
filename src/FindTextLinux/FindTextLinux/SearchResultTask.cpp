#include "SearchResultTask.h"
#include "SearchPathTask.h"
#include "FindTextLinux.h"

SearchResultTask::SearchResultTask():
m_task(nullptr)
{

}

void SearchResultTask::DoTask()
{
    m_task->m_client->m_text.append((m_result + "\n").c_str());
}

void SearchResultTask::setParam(const std::string& result, SearchPathTask* task)
{
    m_result = result;
    m_task = task;
}