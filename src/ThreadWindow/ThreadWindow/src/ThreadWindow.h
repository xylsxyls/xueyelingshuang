#pragma once
#include "ThreadWindowMacro.h"
#include <stdint.h>

class ThreadWindowAPI ThreadWindow
{
public:
	ThreadWindow();

public:
	/** 创建窗口，创建好了才会返回
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		//处理指令集
		switch (msg)
		{
		case WM_CREATE:
		{
			break;
		}
		default:
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	*/
	HWND Create(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM), bool visible = false);

	/** 销毁窗口，销毁了退出线程了才会返回
	*/
	void Destroy();

private:
	int32_t m_threadId;
};