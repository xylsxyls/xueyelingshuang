#pragma once
#include "ThreadWindowMacro.h"
#include <stdint.h>
#include "Semaphore/SemaphoreAPI.h"
#include <string>
#include <windows.h>

/** �̴߳�����
*/
class ThreadWindowAPI ThreadWindow
{
public:
	/** ���캯��
	*/
	ThreadWindow();

	/** ��������
	*/
	~ThreadWindow();

public:
	/** �������ڣ��������˲Ż᷵��
	LRESULT __stdcall WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
	//����ָ�
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

	/** ��ȡHWND
	*/
	HWND GetHWND();

	/** ��ȡHDC
	*/
	HDC GetHDC();

	/** �ͷ�HDC
	*/
	void ReleaseHDC();

	/** ���ٴ��ڣ��������˳��߳��˲Ż᷵�أ��ýӿڻ�������������ִ��
	*/
	void Destroy();

private:
	int32_t m_threadId;
	Semaphore m_semaphore;
	HWND m_hWnd;
	HDC m_hdc;
};