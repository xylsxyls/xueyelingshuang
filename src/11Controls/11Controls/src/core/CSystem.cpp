#include "CSystem.h"
#include "CStringManager.h"

POINT CSystem::taskbarRightBottomPoint()
{
	RECT rect = { 0, 0, 0, 0 };
	::GetWindowRect(::FindWindowA("Shell_TrayWnd", ""), &rect);
	RECT screenRect = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

	//分4种情况，任务栏可能出现在左侧上侧右侧底侧
	//右侧
	if (rect.left != screenRect.left)
	{
		POINT point = { rect.left, screenRect.bottom };
		return point;
	}
	//底侧
	else if (rect.top != screenRect.top)
	{
		POINT point = { screenRect.right, rect.top };
		return point;
	}
	//上侧左侧和其余情况一律从右下角弹出
	else
	{
		POINT point = { screenRect.right, screenRect.bottom };
		return point;
	}
}

POINT CSystem::screenCenterPoint()
{
	qint32 screenwidth_real = ::GetSystemMetrics(SM_CXSCREEN);
	qint32 screenheight_real = ::GetSystemMetrics(SM_CYSCREEN);
	POINT point = { screenwidth_real / 2, screenheight_real / 2 };
	return point;
}

bool CSystem::isMouseLeftDown()
{
	return ((::GetAsyncKeyState(MOUSE_MOVED) & 0x8000) ? 1 : 0);
}

bool CSystem::isMouseRightDown()
{
	return ((::GetAsyncKeyState(MOUSE_EVENT) & 0x8000) ? 1 : 0);
}

bool CSystem::isMouseMidDown()
{
	return ((::GetAsyncKeyState(MOUSE_WHEELED) & 0x8000) ? 1 : 0);
}

QRect CSystem::rectValid(const QRect& rect)
{
    QRect result = rect;
    if (result.left() < 0)
    {
        result.setLeft(0);
    }
    if (result.top() < 0)
    {
        result.setTop(0);
    }
    if (result.right() < 0)
    {
        result.setRight(0);
    }
    if (result.bottom() < 0)
    {
        result.setBottom(0);
    }
    return result;
}

std::string CSystem::GetCurrentExePath()
{
	char szFilePath[1024] = {};
	GetModuleFileNameA(NULL, szFilePath, 1024);
	return CSystem::GetName(szFilePath, 4);
}

std::string CSystem::GetName(const std::string& path, qint32 flag)
{
	qint32 left = (qint32)path.find_last_of("/\\");
	std::string name = CStringManager::Mid(path, left + 1, path.length() - left - 1);
	qint32 point = (qint32)name.find_last_of(".");
	switch (flag)
	{
	case 1:
	{
		return name;
	}
	case 2:
	{
		return CStringManager::Mid(name, point + 1, point == -1 ? 0 : name.length() - point - 1);
	}
	case 3:
	{
		return CStringManager::Mid(name, 0, point == -1 ? name.length() : point);
	}
	case 4:
	{
		return CStringManager::Mid(path, 0, left + 1);
	}
	default:
		return "";
	}
}