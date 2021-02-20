#include "SearchFileTask.h"
#include "Ctxt/CtxtAPI.h"
#include "SearchPathTask.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SearchResultTask.h"
#include <string.h>
#include <fstream>
#include <locale>
#include <codecvt>

SearchFileTask::SearchFileTask():
m_task(nullptr)
{

}

void SearchFileTask::DoTask()
{
    std::string result;
    if (m_isPathFind)
    {
        result += "-------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        result += (m_path + "\n\n");
    }

    Ctxt file(m_path);
    file.LoadTxt(Ctxt::ONE_LINE);
    getUnicodeTxt();
    int32_t index = -1;
    while (index++ != file.m_vectxt.size() - 1)
    {
        std::string& line = file.m_vectxt[index][0];
        std::string charset;
        int32_t charsetNum = 0;
        if (m_task->m_charset == "GBK")
        {
            charsetNum = 1;
        }
        else if (m_task->m_charset == "unicode")
        {
            charsetNum = 2;
        }
        else if (m_task->m_charset == "UTF-8")
        {
            charsetNum = 3;
        }
        switch (charsetNum)
        {
        case 1:
        {
            std::string gbkLine;
            std::string gbkLineNotMatchCase;
            getGbkLine(index, line, gbkLine, gbkLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? gbkLine : gbkLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "GBK";
                line = gbkLine;
                break;
            }
            std::string unicodeLine;
            std::string unicodeLineNotMatchCase;
            getUnicodeLine(index, unicodeLine, unicodeLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? unicodeLine : unicodeLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "unicode";
                line = unicodeLine;
                break;
            }
            std::string utf8LineNotMatchCase;
            getUtf8Line(line, utf8LineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? line : utf8LineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "UTF-8";
                break;
            }
        }
        break;
        case 2:
        {
            std::string unicodeLine;
            std::string unicodeLineNotMatchCase;
            getUnicodeLine(index, unicodeLine, unicodeLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? unicodeLine : unicodeLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "unicode";
                line = unicodeLine;
                break;
            }
            std::string gbkLine;
            std::string gbkLineNotMatchCase;
            getGbkLine(index, line, gbkLine, gbkLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? gbkLine : gbkLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "GBK";
                line = gbkLine;
                break;
            }
            std::string utf8LineNotMatchCase;
            getUtf8Line(line, utf8LineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? line : utf8LineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "UTF-8";
                break;
            }
        }
        break;
        case 3:
        {
            std::string utf8LineNotMatchCase;
            getUtf8Line(line, utf8LineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? line : utf8LineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "UTF-8";
                break;
            }
            std::string gbkLine;
            std::string gbkLineNotMatchCase;
            getGbkLine(index, line, gbkLine, gbkLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? gbkLine : gbkLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "GBK";
                line = gbkLine;
                break;
            }
            std::string unicodeLine;
            std::string unicodeLineNotMatchCase;
            getUnicodeLine(index, unicodeLine, unicodeLineNotMatchCase);
            if (CStringManager::Find(m_task->m_isMatchCase ? unicodeLine : unicodeLineNotMatchCase, m_search) != std::string::npos)
            {
                charset = "unicode";
                line = unicodeLine;
                break;
            }
        }
        break;
        default:
            break;
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
        result += ("line:" + std::to_string(index + 1) + "£¬" + charset + "£º" + line.c_str() + "\n");
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
    bool isPathFind,
    const std::string& search,
    SearchPathTask* task)
{
    m_path = path;
    m_isPathFind = isPathFind;
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

void SearchFileTask::getGbkLine(int32_t lineIndex, const std::string& line, std::string& gbkLine, std::string& gbkLineNotMatchCase)
{
    gbkLine = CStringManager::AnsiToUtf8(line);
    if (!m_task->m_isMatchCase)
    {
        gbkLineNotMatchCase = CStringManager::MakeLower(gbkLine);
    }
}

void SearchFileTask::getUnicodeLine(int32_t lineIndex, std::string& unicodeLine, std::string& unicodeLineNotMatchCase)
{
    if (m_wvectxt.size() <= lineIndex)
    {
        return;
    }
    unicodeLine = CStringManager::UnicodeToUtf8(m_wvectxt[lineIndex][0]);
    if (!m_task->m_isMatchCase)
    {
        unicodeLineNotMatchCase = CStringManager::MakeLower(unicodeLine);
    }
}

void SearchFileTask::getUtf8Line(const std::string& line, std::string& utf8LineNotMatchCase)
{
    if (!m_task->m_isMatchCase)
    {
        utf8LineNotMatchCase = CStringManager::MakeLower(line);
    }
}

void SearchFileTask::getUnicodeTxt()
{
	std::wifstream myfile(m_path);
	if (!myfile.is_open())
	{
		return;
	}

	myfile.imbue(std::locale(myfile.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));

	std::wstring strLine;
	std::vector<std::wstring> vecLine;
    bool hasLine = true;
	do 
	{
        hasLine = (bool)getline(myfile, strLine);
#ifdef __unix__
		if((!(strLine.empty())) && (strLine.back() == '\r'))
		{
			strLine.pop_back();
		}
#endif
		vecLine.clear();
		vecLine.push_back(strLine);
		m_wvectxt.push_back(vecLine);
	} while (!myfile.eof() && hasLine);
	myfile.close();
	myfile.clear();
}