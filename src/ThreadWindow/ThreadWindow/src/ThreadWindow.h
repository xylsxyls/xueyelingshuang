#pragma once
#include "ThreadWindowMacro.h"
#include <stdint.h>
#include "Semaphore/SemaphoreAPI.h"
#include <string>
#include <windows.h>

/** 线程窗口类
*/
class ThreadWindowAPI ThreadWindow
{
public:
	/** 构造函数
	*/
	ThreadWindow();

	/** 析构函数
	*/
	~ThreadWindow();

public:
	/** 创建窗口，创建好了才会返回
	LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
	HWND Create(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM),
		bool visible = false,
		int32_t left = 100,
		int32_t top = 100,
		int32_t width = 640,
		int32_t height = 480,
		bool hasTitle = true,
		const std::string& windowName = "Win32ThreadWindow");

	/** 获取HWND
	*/
	HWND GetHWND();

	/** 获取HDC
	*/
	HDC GetHDC();

	/** 释放HDC
	*/
	void ReleaseHDC();

	/** 销毁窗口，销毁了退出线程了才会返回，该接口会在析构函数中执行
	*/
	void Destroy();

private:
	int32_t m_threadId;
	Semaphore m_semaphore;
	HWND m_hWnd;
	HDC m_hdc;
};