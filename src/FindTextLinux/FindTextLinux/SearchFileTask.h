#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>

class SearchPathTask;

class SearchFileTask : public CTask
{
public:
    SearchFileTask();

public:
    void DoTask();

    void setParam(const std::string& path,
        bool isPathFind,
        const std::string& search,
        SearchPathTask* task);

	int32_t getUtf8Offset(int32_t lineIndex, const std::string& line);

	int32_t getUnicodeOffset(int32_t lineIndex, const std::string& line);

    void getGbkLine(int32_t lineIndex, const std::string& line, std::string& gbkLine, std::string& gbkLineNotMatchCase);

    void getUnicodeLine(int32_t lineIndex, std::string& unicodeLine, std::string& unicodeLineNotMatchCase);

    void getUtf8Line(const std::string& line, std::string& utf8LineNotMatchCase);

    void getUnicodeTxt();

private:
    std::string m_path;
    bool m_isPathFind;
	std::string m_search;
	SearchPathTask* m_task;
    std::vector<std::vector<std::wstring>> m_wvectxt;
};