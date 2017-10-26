#pragma once
#include <windows.h>
#include <stdint.h>
#include "CHookMacro.h"

class CHookAPI CHook
{
public:
	/*回调函数

	LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
	{
		// 请在这里添加消息处理代码
	
		// 将事件传递到下一个钩子
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	*/

	/** 初始化传入需要钩出的消息
	@param [in] hookId 需要钩出的ID号，比如WH_MOUSE，WH_KEYBOARD，WH_GETMESSAGE
	*/
	static void Init(int32_t hookId, HOOKPROC hookFun);
	static void Uninit();

	static const DWORD& GetVkCode(LPARAM lParam);
	static bool IsKeyUp(WPARAM wParam);
	static bool IsKeyDown(WPARAM wParam);

	static HHOOK s_hHook;
	static HINSTANCE s_hInstDLL;
};