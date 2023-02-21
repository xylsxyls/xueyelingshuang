#pragma once
#include <windows.h>
#include <stdint.h>
#include "CHookMacro.h"

class CHookAPI CHook
{
public:
	/*�ص�����

	LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
	{
		// �������������Ϣ�������
	
		// ���¼����ݵ���һ������
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	*/

	/** ��ʼ��������Ҫ��������Ϣ
	@param [in] hookId ��Ҫ������ID�ţ�����WH_MOUSE��WH_KEYBOARD��WH_GETMESSAGE
	*/
	static void Init(int32_t hookId, HOOKPROC hookFun);
	static void Uninit();

	static POINT GetMousePosition(LPARAM lParam);

	static const DWORD& GetVkCode(LPARAM lParam);
	static bool IsKeyUp(WPARAM wParam);
	static bool IsKeyDown(WPARAM wParam);

	static HHOOK s_hHook;
	static HINSTANCE s_hInstDLL;
};