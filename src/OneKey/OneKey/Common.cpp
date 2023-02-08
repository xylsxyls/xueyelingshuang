#include "stdafx.h"
#include "Common.h"
#include "CSystem/CSystemAPI.h"

std::string Common::heroHeadPath()
{
	return CSystem::GetCurrentExePath() + "HeroHead\\";
}

void Common::deleteAllHeroHead(bool isDestroyDir)
{
	std::string path = heroHeadPath();
	if (CSystem::DirOrFileExist(path))
	{
		int32_t index = 99;
		while (index++ != 600 - 1)
		{
			CSystem::deleteFile((path + std::to_string(index) + ".jpg").c_str());
		}
		if (isDestroyDir)
		{
			CSystem::DestroyDir(path);
		}
	}
}