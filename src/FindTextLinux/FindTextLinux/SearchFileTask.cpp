#include "SearchFileTask.h"
#include "Ctxt/CtxtAPI.h"
#include "SearchPathTask.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SearchResultTask.h"
#include <string.h>

SearchFileTask::SearchFileTask():
m_task(nullptr)
{

}

void SearchFileTask::DoTask()
{
    std::string result;
    Ctxt file(m_path);
    file.LoadTxt(Ctxt::ONE_LINE);
    int32_t index = -1;
    while (index++ != file.m_vectxt.size() - 1)
    {
        std::string& line = file.m_vectxt[index][0];
        if (!m_task->m_isMatchCase)
        {
            line = CStringManager::MakeLower(line);
        }
        std::string charset;
        if (m_task->m_charset == "GBK")
        {
            if (CStringManager::Find(line, m_search) != std::string::npos)
            {
                charset = "GBK";
            }
            else
            {
                std::string unicodeLine;
                getUnicodeLine(index, line, unicodeLine);
                if (CStringManager::Find(unicodeLine, m_search) != std::string::npos)
                {
                    charset = "unicode";
                }
                else
                {
                    std::string utf8Line;
                    getUtf8Line(index, line, utf8Line);
                    if (CStringManager::Find(utf8Line, m_search) != std::string::npos)
                    {
                        charset = "UTF-8";
                    }
                }
            }
        }
        else if (m_task->m_charset == "unicode")
        {
            std::string unicodeLine;
            getUnicodeLine(index, line, unicodeLine);
            if (CStringManager::Find(unicodeLine, m_search) != std::string::npos)
            {
                charset = "unicode";
            }
            else
            {
                if (CStringManager::Find(line, m_search) != std::string::npos)
                {
                    charset = "GBK";
                }
                else
                {
                    std::string utf8Line;
                    getUtf8Line(index, line, utf8Line);
                    if (CStringManager::Find(utf8Line, m_search) != std::string::npos)
                    {
                        charset = "UTF-8";
                    }
                }
            }
        }
        else if (m_task->m_charset == "UTF-8")
        {
            std::string utf8Line;
            getUtf8Line(index, line, utf8Line);
            if (CStringManager::Find(utf8Line, m_search) != std::string::npos)
            {
                charset = "UTF-8";
            }
            else
            {
                if (CStringManager::Find(line, m_search) != std::string::npos)
                {
                    charset = "GBK";
                }
                else
                {
                    std::string unicodeLine;
                    getUnicodeLine(index, line, unicodeLine);
                    if (CStringManager::Find(unicodeLine, m_search) != std::string::npos)
                    {
                        charset = "unicode";
                    }
                }
            }
        }
        if (charset.empty())
        {
            continue;
        }
        if (result.empty())
        {
            result += "-------------------------------------------------------------------------------------------------------------------------------------------------------\n";
            result += (m_path + "\n\n");
        }
        result += ("line:" + std::to_string(index + 1) + "£¬" + charset + "£º" + line + "\n");
    }
    if (result.empty())
    {
        return;
    }
    std::shared_ptr<SearchResultTask> spSearchResultTask(new SearchResultTask);
    spSearchResultTask->setParam(result, m_task);
    CTaskThreadManager::Instance().GetThreadInterface(m_task->m_searchResultThreadId)->PostTask(spSearchResultTask);
}

void SearchFileTask::setParam(const std::string& path,
    const std::string& search,
    SearchPathTask* task)
{
    m_path = path;
    m_search = search;
    m_task = task;
}

int32_t SearchFileTask::getUtf8Offset(int32_t lineIndex, const std::string& line)
{
    if (lineIndex == 0
		&& line.length() >= 4
		&& line[0] == (char)0xEF
		&& line[1] == (char)0xBB
		&& line[2] == (char)0xBF)
	{
		return 3;
	}
	return 0;
}

int32_t SearchFileTask::getUnicodeOffset(int32_t lineIndex, const std::string& line)
{
    int32_t offset = 0;
	if (lineIndex == 0)
	{
		if (line.length() >= 3)
		{
			if ((line[0] == (char)0xFE && line[1] == (char)0xFF)
				|| (line[0] == (char)0xFF && line[1] == (char)0xFE))
			{
				offset = 2;
				return offset;
			}
		}
		if (line.length() >= 5)
		{
			if ((line[0] == 0 && line[1] == 0 && line[2] == (char)0xFE && line[3] == (char)0xFF)
				|| (line[0] == 0 && line[1] == 0 && line[2] == (char)0xFF && line[3] == (char)0xFE))
			{
				offset = 4;
				return offset;
			}
		}
	}
	else
	{
		if (line.length() >= 2)
		{
			if (line[0] == 0)
			{
				offset = 1;
				return offset;
			}
		}
	}
	return offset;
}

void SearchFileTask::getUnicodeLine(int32_t lineIndex, const std::string& line, std::string& unicodeLine)
{
    char* szLineBk = new char[line.length() + 4];
	::memcpy(szLineBk, line.c_str(), line.length());
	szLineBk[line.length() + 0] = 0;
	szLineBk[line.length() + 1] = 0;
	szLineBk[line.length() + 2] = 0;
	szLineBk[line.length() + 3] = 0;
	unicodeLine = CStringManager::UnicodeToAnsi((wchar_t*)&szLineBk[getUnicodeOffset(lineIndex, line)]);
    delete[] szLineBk;
}

void SearchFileTask::getUtf8Line(int32_t lineIndex, const std::string& line, std::string& utf8Line)
{
    utf8Line = CStringManager::Utf8ToAnsi(&line[0] + getUtf8Offset(lineIndex, line));
}