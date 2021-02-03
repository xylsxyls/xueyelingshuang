#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

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

    void getGbkLine(int32_t lineIndex, const std::string& line, std::string& gbkLine);

    void getUnicodeLine(int32_t lineIndex, const std::string& line, std::string& unicodeLine);

    void getUtf8Line(int32_t lineIndex, const std::string& line, std::string& utf8Line);

private:
    std::string m_path;
    bool m_isPathFind;
	std::string m_search;
	SearchPathTask* m_task;
};