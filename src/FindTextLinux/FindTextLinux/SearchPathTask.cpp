#include "SearchPathTask.h"
#include "CSystem/CSystemAPI.h"
#include "FindTextLinux.h"
#include "CStringManager/CStringManagerAPI.h"
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
            if (isFind)
            {
                m_task->m_client->m_text += "-------------------------------------------------------------------------------------------------------------------------------------------------------\n";
                m_task->m_client->m_text += (QString::fromStdString(path) + "\n\n\n");
                return m_task->m_exit;
            }
        }
        else
        {
            static int32_t searchFileThreadIndex = -1;
            std::shared_ptr<SearchFileTask> spSearchFileTask(new SearchFileTask);
            spSearchFileTask->setParam(path, isFind, search, m_task);
            int32_t threadIndex = ++searchFileThreadIndex % m_task->m_vecSearchFileThreadId.size();
            uint32_t threadId = m_task->m_vecSearchFileThreadId[threadIndex];
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
m_client(nullptr),
m_searchResultThreadId(0)
{

}

void SearchPathTask::DoTask()
{
    m_client->m_text.clear();
    if ((!CSystem::DirOrFileExist(m_path)) || (m_searchFormat && m_format.empty()))
    {
        emit m_client->searchEnd();
        return;
    }
    
    int32_t index = -1;
    while (index++ != m_client->m_coreCount * 2 - 1)
    {
        m_vecSearchFileThreadId.push_back(CTaskThreadManager::Instance().Init());
    }
    m_searchResultThreadId = CTaskThreadManager::Instance().Init();

    CStringManager::split(m_vecFormat, m_format, '.');
    if (!m_vecFormat.empty())
    {
        m_vecFormat.erase(m_vecFormat.begin());
    }
    std::vector<std::string> vecPath = CSystem::findFilePath(m_path, 3, "", EveryFilePath(this));
    if (m_exit)
    {
        int32_t index = -1;
        while (index++ != m_vecSearchFileThreadId.size() - 1)
        {
            CTaskThreadManager::Instance().Uninit(m_vecSearchFileThreadId[index]);
        }
        m_vecSearchFileThreadId.clear();
        CTaskThreadManager::Instance().Uninit(m_searchResultThreadId);
        m_searchResultThreadId = 0;
        return;
    }
    index = -1;
    while (index++ != m_vecSearchFileThreadId.size() - 1)
    {
        CTaskThreadManager::Instance().WaitForEnd(m_vecSearchFileThreadId[index]);
    }
    m_vecSearchFileThreadId.clear();
    CTaskThreadManager::Instance().WaitForEnd(m_searchResultThreadId);
    m_searchResultThreadId = 0;
    emit m_client->searchEnd();
}

void SearchPathTask::StopTask()
{
    m_exit = true;
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