#include "SearchPathTask.h"
#include "CSystem/CSystemAPI.h"
#include "FindTextLinux.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SendToMessageTest.h"
#include "SearchFileTask.h"

class EveryFilePath
{
public:
    EveryFilePath(SearchPathTask* task):
        m_task(task)
    {

    }

    ~EveryFilePath()
    {
	    m_task = nullptr;
    }

public:
    bool operator()(const std::string& path)
    {
        RCSend("path = %s", path.c_str());

        std::string suffix = CSystem::GetName(path, 2);
        bool hasFormat = (std::find(m_task->m_vecFormat.begin(), m_task->m_vecFormat.end(), suffix) != m_task->m_vecFormat.end());
        if ((m_task->m_searchFormat && !hasFormat) || (!m_task->m_searchFormat && hasFormat))
        {
            return m_task->m_exit;
        }
        std::string fileName = CSystem::GetName(path, 1);
        std::string search = m_task->m_search;
        if (!m_task->m_hasSuffix)
        {
            fileName = CSystem::GetName(path, 3);
        }
        if (!m_task->m_isMatchCase)
        {
            fileName = CStringManager::MakeLower(fileName);
            search = CStringManager::MakeLower(search);
        }
        bool isFind = (CStringManager::Find(fileName, search) != std::string::npos);
        if (m_task->m_isSearchName)
        {
            m_task->m_client->m_text += (QString::fromStdString(path) + "\n");
            return m_task->m_exit;
        }
        else
        {
            static int32_t searchFileThreadIndex = -1;
            std::shared_ptr<SearchFileTask> spSearchFileTask(new SearchFileTask);
            spSearchFileTask->setParam();
            int32_t threadIndex = ++searchFileThreadIndex % m_task->m_client->m_vecSearchFileThreadId.size();
            uint32_t threadId = m_task->m_client->m_vecSearchFileThreadId[threadIndex];
            CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spSearchFileTask);
        }
        return m_task->m_exit;
    }

private:
    SearchPathTask* m_task;
};

SearchPathTask::SearchPathTask():
m_exit(false),
m_searchFormat(false),
m_hasSuffix(false),
m_isMatchCase(false),
m_isSearchName(false),
m_client(nullptr)
{

}

void SearchPathTask::DoTask()
{
    if ((!CSystem::DirOrFileExist(m_path)) || (m_searchFormat && m_format.empty()))
    {
        m_client->m_text.clear();
        emit m_client->searchEnd();
        return;
    }
    CStringManager::split(m_vecFormat, m_format, '.');
    std::vector<std::string> vecPath = CSystem::findFilePath(m_path, 3, "", EveryFilePath(this));
    RCSend("vecPath.size() = %d", vecPath.size());
}

void SearchPathTask::setParam(const std::string& path,
    bool searchFormat,
    const std::string& format,
    const std::string& search,
    bool hasSuffix,
    bool isMatchCase,
    bool isSearchName,
    const std::string& charset,
    FindTextLinux* client)
{
    m_path = path;
    m_searchFormat = searchFormat;
	m_format = format;
	m_search = search;
	m_hasSuffix = hasSuffix;
	m_isMatchCase = isMatchCase;
	m_isSearchName = isSearchName;
	m_charset = charset;
    m_client = client;
}