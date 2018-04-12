#include "CSystem.h"

POINT CSystem::taskbarRightBottomPoint()
{
	RECT rect = { 0, 0, 0, 0 };
	::GetWindowRect(::FindWindowA("Shell_TrayWnd", ""), &rect);
	RECT screenRect = { 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) };

	//��4����������������ܳ���������ϲ��Ҳ�ײ�
	//�Ҳ�
	if (rect.left != screenRect.left)
	{
		POINT point = { rect.left, screenRect.bottom };
		return point;
	}
	//�ײ�
	else if (rect.top != screenRect.top)
	{
		POINT point = { screenRect.right, rect.top };
		return point;
	}
	//�ϲ������������һ�ɴ����½ǵ���
	else
	{
		POINT point = { screenRect.right, screenRect.bottom };
		return point;
	}
}

POINT CSystem::screenCenterPoint()
{
	int32_t screenwidth_real = ::GetSystemMetrics(SM_CXSCREEN);
	int32_t screenheight_real = ::GetSystemMetrics(SM_CYSCREEN);
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