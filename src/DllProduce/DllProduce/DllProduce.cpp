#include "DllProduce.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <stdint.h>
#include "Ctxt/CtxtAPI.h"

#define SPACE std::string(" ")
#define MARK(src) ("\"" + src + "\"")

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

	std::string changeClassName = CSystem::GetCurrentExePath();
	changeClassName.pop_back();
	changeClassName = CStringManager::Right(changeClassName, changeClassName.size() - changeClassName.find_last_of("/\\") - 1);
	std::string strPath = CSystem::GetCurrentExePath();
	std::vector<std::string> vecPath = CSystem::findFilePath(strPath);
	std::vector<std::string> vecPathBk = vecPath;
	int32_t index = -1;
	while (index++ != vecPath.size() - 1)
	{
		CStringManager::Replace(vecPath[index], changeClassName, className);
		if (vecPath[index].find("DllProduce1.1.exe") != -1)
		{
			continue;
		}
		system(("echo f | xcopy" + SPACE + "/y /i /r /s" + SPACE + MARK(vecPathBk[index]) + SPACE + MARK(vecPath[index])).c_str());
		std::string strEnd = CSystem::GetName(vecPath[index], 2);
		if (strEnd == "h" ||
			strEnd == "cpp" ||
			strEnd == "bat" ||
			strEnd == "py" ||
			strEnd == "vcxproj" ||
			strEnd == "filters" || 
			strEnd == "sln")
		{
			Ctxt txt(vecPath[index]);
			txt.LoadTxt(Ctxt::Load::ONE_LINE);
			int32_t line = -1;
			while (line++ != txt.m_vectxt.size() - 1)
			{
				CStringManager::Replace(txt.m_vectxt[line][0], changeClassName, className);
			}
			txt.Save();
		}
		int32_t relyIndex = -1;
		while (relyIndex++ != vecRelyClassName.size() - 1)
		{
			if (vecPath[index].find(std::string() + "before_" + className + ".bat") != -1)
			{
				Ctxt txt(vecPath[index]);
				txt.LoadTxt(Ctxt::Load::ONE_LINE);
				std::vector<std::vector<std::string>> vectxtBk = txt.m_vectxt;
				int32_t line = -1;
				while (line++ != vectxtBk.size() - 1)
				{
					CStringManager::Replace(vectxtBk[line][0], "DllRelyTest", vecRelyClassName[relyIndex]);
				}
				//替换后把前15行添加到末尾
				int32_t lineBk = -1;
				while (lineBk++ != vectxtBk.size() - 1)
				{
					if (lineBk == 15)
					{
						break;
					}
					txt.m_vectxt.push_back(vectxtBk[lineBk]);
				}
				txt.Save();
			}
		}
	}
	
	return 0;
}