#include "DllProduce.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdint.h>
#include "Ctxt/CtxtAPI.h"

#define SPACE std::string(" ")
#define MARK(src) ("\"" + std::string(src) + "\"")

int32_t main()
{
#ifdef _MSC_VER
	printf("输入目标类名\n");
#elif __unix__
	printf(CStringManager::AnsiToUtf8("输入目标类名\n").c_str());
#endif
	char className[1024] = {};
	scanf("%s", className);
#ifdef _MSC_VER
	printf("输入依赖类名，英文逗号分隔\n");
#elif __unix__
	printf(CStringManager::AnsiToUtf8("输入依赖类名，英文逗号分隔\n").c_str());
#endif
	
	char relyClassName[1024] = {};
	scanf("%s", relyClassName);
	std::vector<std::string> vecRelyClassName = CStringManager::split(relyClassName, ",");
	if (CStringManager::MakeLower(relyClassName) == "none")
	{
		vecRelyClassName.clear();
	}

#ifdef _MSC_VER
	std::string pathLevel = "\\";
#elif __unix__
	std::string pathLevel = "/";
#endif

	std::string currentPath = CSystem::GetCurrentExePath();
	currentPath.pop_back();
	std::string changeClassName = CSystem::GetName(currentPath, 1);
	std::string changeRelyClassName = "DllRelyTest";
	std::string dllTestPath = CSystem::GetCurrentExePath();
	std::string newClassPath = dllTestPath;
	newClassPath.pop_back();
	newClassPath = CSystem::GetName(newClassPath, 4) + className + pathLevel;

#ifdef _MSC_VER
	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(dllTestPath + "*") + SPACE + MARK(newClassPath)).c_str());
#elif __unix__
	system(("cp" + SPACE + "-r" + SPACE + MARK(dllTestPath) + SPACE + MARK(newClassPath)).c_str());
#endif
	CSystem::deleteFile((newClassPath + "DllProduce1.2.exe").c_str());
	CSystem::deleteFile((newClassPath + "DllProduce1.2").c_str());

#ifdef _MSC_VER
	std::string fileReplaceCommandHead = "call" + SPACE + MARK("%FILE_REPLACE%");
#elif __unix__
	std::string fileReplaceCommandHead = MARK("$FILE_REPLACE");
#endif
	system((fileReplaceCommandHead + SPACE + "-dir" + SPACE + newClassPath + SPACE + "-replace" + SPACE + changeClassName + SPACE + className).c_str());
	system((fileReplaceCommandHead + SPACE + "-dir" + SPACE + newClassPath + SPACE + "-name" + SPACE + changeClassName + SPACE + className + SPACE + "1").c_str());

	CSystem::rename(newClassPath + changeClassName, newClassPath + className);

	std::string beforeShPath = newClassPath + "scripts" + pathLevel + "before_" + className + ".sh";
	Ctxt beforeSh(beforeShPath);
	beforeSh.LoadTxt(Ctxt::Load::ONE_LINE);
    // Insert generated dependencies before the final reply; older templates keep their original append behavior.
    std::vector<std::string> replyLine;
    if (!beforeSh.m_vectxt.empty() && !beforeSh.m_vectxt.back().empty() &&
        beforeSh.m_vectxt.back()[0].find("CLOUD_REBUILD_LIBRARIESRELY") != std::string::npos)
    {
        replyLine = beforeSh.m_vectxt.back();
        beforeSh.m_vectxt.pop_back();
        // 模板末行前的空行随回传行取出，依赖块和回传行各自保留一个分隔空行。
        if (!beforeSh.m_vectxt.empty() && (beforeSh.m_vectxt.back().empty() || beforeSh.m_vectxt.back()[0].empty()))
        {
            beforeSh.m_vectxt.pop_back();
        }
    }
	std::vector<std::string> emptyLine;
	emptyLine.push_back("");
	int32_t index = -1;
	while (index++ != vecRelyClassName.size() - 1)
	{
		beforeSh.m_vectxt.push_back(emptyLine);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[3]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[4]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[5]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[6]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[7]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[8]);
        beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[9]);
        beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[10]);
        beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[11]);
	}
    if (!replyLine.empty())
    {
        beforeSh.m_vectxt.push_back(emptyLine);
        beforeSh.m_vectxt.push_back(replyLine);
    }
	beforeSh.Save();
	
	index = -1;
	while (index++ != vecRelyClassName.size() - 1)
	{
		system((fileReplaceCommandHead + SPACE + "-file" + SPACE + beforeShPath + SPACE + "-replace" + SPACE + changeRelyClassName + SPACE + vecRelyClassName[index] + SPACE + std::to_string(14 + index * 10) + SPACE + std::to_string(23 + index * 10)).c_str());
	}

	system((fileReplaceCommandHead + SPACE + "-files" + SPACE + newClassPath + SPACE + ".sh" + SPACE + "1" + SPACE + "-enter" + SPACE + "0").c_str());

	return 0;
}