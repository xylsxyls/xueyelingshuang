#include "DllProduce.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdint.h>
#include "Ctxt/CtxtAPI.h"

#define SPACE std::string(" ")
#define MARK(src) ("\"" + std::string(src) + "\"")

int32_t main()
{
	printf("输入目标类名\n");
	char className[1024] = {};
	scanf("%s", className);
	printf("输入依赖类名，英文逗号分隔\n");
	char relyClassName[1024] = {};
	scanf("%s", relyClassName);
	std::vector<std::string> vecRelyClassName = CStringManager::split(relyClassName, ",");
	if (CStringManager::MakeLower(relyClassName) == "none")
	{
		vecRelyClassName.clear();
	}

	std::string changeClassName = "DllTest";
	std::string changeRelyClassName = "DllRelyTest";
	std::string dllTestPath = CSystem::GetCurrentExePath();
	std::string newClassPath = dllTestPath;
	newClassPath.pop_back();
	newClassPath = CSystem::GetName(newClassPath, 4) + className + "\\";

	system(("xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(dllTestPath + "*") + SPACE + MARK(newClassPath)).c_str());
	CSystem::deleteFile((newClassPath + "DllProduce1.2.exe").c_str());
	CSystem::deleteFile((newClassPath + "DllProduce1.2").c_str());

	system(("call" + SPACE + MARK("%FILE_REPLACE%") + SPACE + "-dir" + SPACE + newClassPath + SPACE + "-replace" + SPACE + changeClassName + SPACE + className).c_str());
	system(("call" + SPACE + MARK("%FILE_REPLACE%") + SPACE + "-dir" + SPACE + newClassPath + SPACE + "-name" + SPACE + changeClassName + SPACE + className + SPACE + "1").c_str());

	CSystem::rename(newClassPath + changeClassName, newClassPath + className);

	std::string beforeShPath = newClassPath + "scripts\\before_" + className + ".sh";
	Ctxt beforeSh(beforeShPath);
	beforeSh.LoadTxt(Ctxt::Load::ONE_LINE);
	std::vector<std::string> emptyLine;
	emptyLine.push_back("\r");
	int32_t index = -1;
	while (index++ != vecRelyClassName.size() - 1)
	{
		beforeSh.m_vectxt.push_back(emptyLine);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[2]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[3]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[4]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[5]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[6]);
		beforeSh.m_vectxt.push_back(beforeSh.m_vectxt[7]);
	}
	beforeSh.Save();
	
	index = -1;
	while (index++ != vecRelyClassName.size() - 1)
	{
		system(("call" + SPACE + MARK("%FILE_REPLACE%") + SPACE + "-file" + SPACE + beforeShPath + SPACE + "-replace" + SPACE + changeRelyClassName + SPACE + vecRelyClassName[index] + SPACE + std::to_string(10 + index * 7) + SPACE + std::to_string(16 + index * 7)).c_str());
	}

	return 0;
}