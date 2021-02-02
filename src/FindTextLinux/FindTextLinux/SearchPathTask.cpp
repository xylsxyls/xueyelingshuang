#include "SearchPathTask.h"

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