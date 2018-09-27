#include "vsVersionChange.h"
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"

int32_t main()
{
	std::string src = CSystem::GetEnvironment("XUEYELINGSHUANG") + "src\\";
	printf("请输入原始版本：\n");
	std::string srcVersion;
	std::cin >> srcVersion;
	printf("请输入要修改成的版本：\n");
	std::string destVersion;
	std::cin >> destVersion;

	if (srcVersion == "vs2010")
	{
		srcVersion = "100";
	}
	else if (srcVersion == "vs2013")
	{
		srcVersion = "120";
	}

	if (destVersion == "vs2010")
	{
		destVersion = "100";
	}
	else if (destVersion == "vs2013")
	{
		destVersion = "120";
	}

	printf("请输入要修改的类名：\n");
	std::string projects;
	std::cin >> projects;
	std::vector<std::string> vecProject = CStringManager::split(projects, ",");
	bool dlllib = false;
	do 
	{
		int32_t index = -1;
		while (index++ != vecProject.size() - 1)
		{
			std::string& project = vecProject[index];
			std::string pythonPath = src + project + "\\scripts\\rebuild_" + (dlllib ? "dll" : "lib") + "_" + project + ".py";
			std::string vcxprojPath = src + project + "\\" + project + "\\vcxproj\\" + project + (dlllib ? "" : "_lib") + ".vcxproj";
			Ctxt* txt = new Ctxt(pythonPath);
			txt->LoadTxt(3, "");
			txt->Replace("VS" + srcVersion + "COMNTOOLS", "VS" + destVersion + "COMNTOOLS");
			txt->Save();
			delete txt;
			txt = new Ctxt(vcxprojPath);
			txt->LoadTxt(3, "");
			txt->Replace("<PlatformToolset>v" + srcVersion + "</PlatformToolset>", "<PlatformToolset>v" + destVersion + "</PlatformToolset>");
			txt->Save();
			delete txt;
		}
		dlllib = !dlllib;
	} while (dlllib);

	getchar();
	return 0;
}