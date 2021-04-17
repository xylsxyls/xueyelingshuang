#include "CHook.h"
#pragma data_seg("HookData")
HHOOK CHook::s_hHook = nullptr;
HINSTANCE CHook::s_hInstDLL = nullptr;
#pragma data_seg()
#pragma comment(linker,"/SECTION:HookData,RWS")

BOOL APIENTRY DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	CHook::s_hInstDLL = hInstDLL;
	return TRUE;
}

void CHook::Init(int32_t hookId, HOOKPROC hookFun)
{
	CHook::s_hHook = SetWindowsHookEx(hookId, hookFun, CHook::s_hInstDLL, 0);
}

void CHook::Uninit()
{
	UnhookWindowsHookEx(CHook::s_hHook);
}

const DWORD& CHook::GetVkCode(LPARAM lParam)
{
	KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *)lParam;
	return pkbhs->vkCode;
}

bool CHook::IsKeyUp(WPARAM wParam)
{
	return wParam == WM_KEYUP;
}

bool CHook::IsKeyDown(WPARAM wParam)
{
	return wParam == WM_KEYDOWN;
}