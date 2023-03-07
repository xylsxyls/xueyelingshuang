#include "ScreenScript.h"
#include "CScreen/CScreenAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include <atlimage.h>
#include "CSystem/CSystemAPI.h"

bool ScreenScript::FindClick(const std::string& path, bool leftClick, bool doubleClick, const xyls::Rect& rect,
	double sim, int32_t sleepTime, const xyls::Rect& move)
{
	xyls::Rect findRect;
	if (rect.empty())
	{
		findRect = CSystem::GetWindowResolution();
	}
	else
	{
		findRect = rect;
	}

	std::string bmpPath = GetBmpPath(path);
	if (bmpPath.empty())
	{
		return false;
	}

	int32_t x = 0;
	int32_t y = 0;
	if (!CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), sim))
	{
		return false;
	}

	CImage img;
	HRESULT res = img.Load(path.c_str());
	if (res != S_OK)
	{
		return false;
	}

	int32_t imgWidth = img.GetWidth();
	int32_t imgHeight = img.GetHeight();
	img.Destroy();

	CMouse::MoveAbsolute(xyls::Rect(x + imgWidth / 3, y + imgHeight / 3, x + imgWidth * 2 / 3, y + imgHeight * 2 / 3), sleepTime);
    if (leftClick && doubleClick)
    {
		CMouse::LeftDoubleClick(sleepTime);
    }
	else if (!leftClick && doubleClick)
	{
		CMouse::RightDoubleClick(sleepTime);
	}
	else if (leftClick && !doubleClick)
	{
		CMouse::LeftClick(sleepTime);
	}
	else if (!leftClick && !doubleClick)
	{
		CMouse::RightClick(sleepTime);
	}
	CMouse::MoveAbsolute(move, sleepTime);
	
	return true;
}

xyls::Rect ScreenScript::FindPic(const std::string& path, const xyls::Rect& rect, double sim)
{
	xyls::Rect findRect;
	if (rect.empty())
	{
		findRect = CSystem::GetWindowResolution();
	}
	else
	{
		findRect = rect;
	}

	std::string bmpPath = GetBmpPath(path);
	if (bmpPath.empty())
	{
		return { xyls::Point(0, 0), 0, 0 };
	}

	int32_t x = 0;
	int32_t y = 0;
	if (!CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), sim))
	{
		return { xyls::Point(0, 0), 0, 0 };
	}
	CImage img;
	HRESULT res = img.Load(path.c_str());
	if (res != S_OK)
	{
		return { xyls::Point(0, 0), 0, 0 };
	}

	int32_t imgWidth = img.GetWidth();
	int32_t imgHeight = img.GetHeight();
	img.Destroy();

	return { xyls::Point{ x, y }, imgWidth, imgHeight };
}

bool ScreenScript::WaitForPic(const std::string& path,
							  const xyls::Rect& rect,
							  double sim,
							  int32_t timeOut,
							  int32_t searchIntervalTime)
{
	int32_t beginTime = ::GetTickCount();
	while ((int32_t)(::GetTickCount() - beginTime) <= timeOut)
	{
		if (!FindPic(path, rect, sim).empty())
		{
			return true;
		}
		CSystem::Sleep(searchIntervalTime);
	}
	return false;
}

bool ScreenScript::WaitClickPic(const std::string& path,
								bool leftClick,
								bool doubleClick,
								const xyls::Rect& rect,
								double sim,
								int32_t timeOut,
								int32_t searchIntervalTime,
								const xyls::Rect& move)
{
	WaitForPic(path, rect, sim, timeOut, searchIntervalTime);
    return FindClick(path, leftClick, doubleClick, rect, sim, -1, move);
}

std::string ScreenScript::GetBmpPath(const std::string& path)
{
	std::string curExePath = CSystem::GetCurrentExePath();
	if (CSystem::DirOrFileExist(curExePath + "ScreenScriptTemp") == false)
	{
		if(!CSystem::CreateDir(curExePath + "ScreenScriptTemp"))
		{
			return "";
		}
	}
	std::string bmpPath = curExePath + "ScreenScriptTemp\\" + CSystem::GetName(path, 3) + ".bmp";
	if (CSystem::DirOrFileExist(bmpPath) == false)
	{
		if (!CScreen::ChangeToBmp(bmpPath, path))
		{
			RCSend("error change to bmp");
			return "";
		}
	}
	return bmpPath;
}

/*
int32_t main()
{
	Sleep(3000);
	ScreenScript::FindClick("D:\\12123.png");
	getchar();
	return 0;
}
*/