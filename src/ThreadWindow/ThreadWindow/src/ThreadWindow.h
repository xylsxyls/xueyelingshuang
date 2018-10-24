#pragma once
#include "ThreadWindowMacro.h"
#include <stdint.h>

class ThreadWindowAPI ThreadWindow
{
public:
	ThreadWindow();

public:
	/** �������ڣ��������˲Ż᷵��
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
	HWND Create(LRESULT (CALLBACK* wndProc)(HWND, UINT, WPARAM, LPARAM), bool visible = false);

	/** ���ٴ��ڣ��������˳��߳��˲Ż᷵��
	*/
	void Destroy();

private:
	int32_t m_threadId;
};