#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include <string>

class FindTextLinux;

class SearchPathTask : public CTask
{
    friend class EveryFilePath;
    friend class SearchFileTask;
    friend class SearchResultTask;
public:
    SearchPathTask();

public:
    void DoTask();

    void StopTask();

    void setParam(const std::string& path,
        bool searchFormat,
        const std::string& format,
        const std::string& search,
        bool hasSuffix,
        bool isMatchCase,
        bool isSearchName,
        const std::string& charset,
        FindTextLinux* client);

private:
    std::string m_path;
    bool m_searchFormat;
	std::string m_format;
	std::string m_search;
	bool m_hasSuffix;
	bool m_isMatchCase;
	bool m_isSearchName;
	std::string m_charset;
    FindTextLinux* m_client;
    std::vector<std::string> m_vecFormat;
    std::vector<uint32_t> m_vecSearchFileThreadId;
    uint32_t m_searchResultThreadId;
    std::atomic<bool> m_exit;
};