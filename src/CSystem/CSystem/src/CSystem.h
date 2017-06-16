#pragma once
#include <windows.h>
#include <string>
#include "CSystemMacro.h"
using namespace std;

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
	//?生成UUID，传入1表示带-，传0表示不带
	static string uuid(int flag = 1);
    //?打开系统文件夹
    static void OpenFolder(const string& folder);
    //?打开系统文件夹并选择文件
    static void OpenFolderAndSelectFile(const string& file);
    //?外部打开文件
    static void OpenFile(const string& file);
    //?外部打开网页
    static void OpenWebPage(const string& webPage);
};