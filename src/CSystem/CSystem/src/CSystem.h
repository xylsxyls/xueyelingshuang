#pragma once
#include <windows.h>
#include "CSystemMacro.h"

class CSystemAPI CSystem{
public:
	//?实测CPU主频
	static double GetCPUSpeedGHz();
	//?获取系统任务栏矩形大小
	static RECT GetTaskbarRect();
	//?获取屏幕分辨率
	static RECT GetWindowResolution();
	//?获取去除任务栏及软件最大化时软件内任务栏后屏幕高度
	static int GetVisibleHeight();
	//?跨平台Sleep，毫秒
	static void Sleep(long long milliseconds);
};