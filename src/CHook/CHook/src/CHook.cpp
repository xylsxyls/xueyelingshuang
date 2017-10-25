#include "CHook.h"
#pragma data_seg("HookData")
#pragma data_seg()
#pragma comment(linker,"/SECTION:HookData,RWS")

HHOOK CHook::s_hHook = nullptr;
HINSTANCE CHook::s_hInstDLL = nullptr;

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