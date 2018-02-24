#include "ScreenScript.h"
#include "CScreen/CScreenAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include <atlimage.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "CGetPath/CGetPathAPI.h"

bool ScreenScript::FindClick(const std::string& path, bool leftClick, bool doubleClick, const xyls::Rect& rect)
{
	CImage img;
	HRESULT res = img.Load(path.c_str());
	if (res != S_OK)
	{
		return false;
	}

	xyls::Rect findRect;
	if (rect == xyls::Rect(0, 0, 0, 0))
	{
		findRect = CSystem::GetWindowResolution();
	}
	else
	{
		findRect = rect;
	}

	int32_t imgWidth = img.GetWidth();
	int32_t imgHeight = img.GetHeight();

	std::string bmpPath = GetBmpPath(path);
	if (bmpPath == "")
	{
		return false;
	}
	
    int32_t x = 0;
    int32_t y = 0;
	CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), 0.7);
    if (x == 0 && y == 0)
    {
		return false;
    }

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
    
	img.Destroy();
	return true;
}

bool ScreenScript::FindPic(const std::string& path, const xyls::Rect& rect)
{
	xyls::Rect findRect;
	if (rect == xyls::Rect(0, 0, 0, 0))
	{
		findRect = CSystem::GetWindowResolution();
	}
	else
	{
		findRect = rect;
	}

	std::string bmpPath = GetBmpPath(path);
	if (bmpPath == "")
	{
		return false;
	}

	int32_t x = 0;
	int32_t y = 0;
	return CScreen::FindPic(findRect, bmpPath, x, y, xyls::Color(0, 0, 0), 0.7);
}

bool ScreenScript::WaitForPic(const std::string& path,
							  const xyls::Rect& rect,
							  int32_t timeOut,
							  int32_t searchIntervalTime)
{
	CStopWatch watch;
	while ((int32_t)watch.GetWatchTime() <= timeOut)
	{
		bool result = FindPic(path, rect);
		if (result)
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
								int32_t searchIntervalTime)
{
	WaitForPic(path, rect, timeOut, searchIntervalTime);
	return FindClick(path, leftClick, doubleClick, rect);
}

std::string ScreenScript::GetBmpPath(const std::string& path)
{
	std::string curExePath = CGetPath::GetCurrentExePath();
	if (CSystem::DirOrFileAccess(curExePath + "ScreenScriptTemp") == false)
	{
		bool result = CSystem::CreateDir(curExePath + "ScreenScriptTemp");
		if (result == false)
		{
			return "";
		}
	}
	std::string bmpPath = curExePath + "ScreenScriptTemp\\" + CGetPath::GetName(path, 3) + ".bmp";
	if (CSystem::DirOrFileAccess(bmpPath) == false)
	{
		bool result = CScreen::ChangeToBmp(bmpPath, path);
		if (result == false)
		{
			return "";
		}
	}
	return bmpPath;
}

int main()
{
	Sleep(3000);
	ScreenScript::FindClick("D:\\12123.png");
	getchar();
	return 0;
}