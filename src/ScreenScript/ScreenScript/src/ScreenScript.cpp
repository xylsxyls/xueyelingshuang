#include "ScreenScript.h"
#include "CScreen/CScreenAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include <atlimage.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "CGetPath/CGetPathAPI.h"

bool ScreenScript::FindClick(const std::string& path, bool leftClick, bool doubleClick, const xyls::Rect& rect, const xyls::Rect& moved)
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

	CMouse::MoveAbsolute(moved, 100);
	int32_t x = 0;
	int32_t y = 0;
	if (!CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), 0.7))
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

	CMouse::MoveAbsolute(xyls::Rect(x + imgWidth / 3, y + imgHeight / 3, x + imgWidth * 2 / 3, y + imgHeight  * 2 / 3));
    if (leftClick && doubleClick)
    {
        CMouse::LeftDoubleClick();
    }
	else if (!leftClick && doubleClick)
	{
		CMouse::RightDoubleClick();
	}
	else if (leftClick && !doubleClick)
	{
		CMouse::LeftClick();
	}
	else if (!leftClick && !doubleClick)
	{
		CMouse::RightClick();
	}
	CMouse::MoveAbsolute(moved, 100);
	
	return true;
}

bool ScreenScript::FindPic(const std::string& path, const xyls::Rect& rect, const xyls::Rect& move)
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

	CMouse::MoveAbsolute(move, 100);
	int32_t x = 0;
	int32_t y = 0;
	return CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), 0.7);
}

bool ScreenScript::WaitForPic(const std::string& path,
							  const xyls::Rect& rect,
							  int32_t timeOut,
							  int32_t searchIntervalTime,
							  const xyls::Rect& move)
{
	CMouse::MoveAbsolute(move, 100);
	CStopWatch watch;
	while ((int32_t)watch.GetWatchTime() <= timeOut)
	{
		if (FindPic(path, rect))
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
								int32_t timeOut,
								int32_t searchIntervalTime,
								const xyls::Rect& move)
{
	WaitForPic(path, rect, timeOut, searchIntervalTime, move);
	return FindClick(path, leftClick, doubleClick, rect);
}

std::string ScreenScript::GetBmpPath(const std::string& path)
{
	std::string curExePath = CGetPath::GetCurrentExePath();
	if (CSystem::DirOrFileAccess(curExePath + "ScreenScriptTemp") == false)
	{
		if(!CSystem::CreateDir(curExePath + "ScreenScriptTemp"))
		{
			return "";
		}
	}
	std::string bmpPath = curExePath + "ScreenScriptTemp\\" + CGetPath::GetName(path, 3) + ".bmp";
	if (CSystem::DirOrFileAccess(bmpPath) == false)
	{
		if (!CScreen::ChangeToBmp(bmpPath, path))
		{
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