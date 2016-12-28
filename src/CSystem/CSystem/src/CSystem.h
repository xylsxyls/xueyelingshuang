#pragma once
#include <afxwin.h>
#include "CSystemMacro.h"

class CSystemAPI CSystem{
public:
	//实测CPU主频
	double GetCPUSpeedGHz();
	//获取系统任务栏矩形大小
	CRect GetTaskbarRect();
	//获取屏幕分辨率
	CRect GetWindowResolution();
	//获取去除任务栏及软件最大化时软件内任务栏后屏幕高度
	int GetVisibleHeight();
};