#pragma once
#include <windows.h>
#include <string>
#include <stdint.h>
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
	//?生成UUID，传入1表示带-，传0表示不带
	static std::string uuid(int flag = 1);
	//?打开系统文件夹
	static void OpenFolder(const std::string& folder);
	//?打开系统文件夹并选择文件
	static void OpenFolderAndSelectFile(const std::string& file);
	//?外部打开文件
	static void OpenFile(const std::string& file);
	//?外部打开网页
	static void OpenWebPage(const std::string& webPage);
	//?复制文件
	static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);
	//?获取当前操作系统位数
	static int GetSystemBits();
	//?禁用重定向
	static void ForbidRedir();
	//?恢复重定向
	static void RecoveryRedir();
	//?获取当前系统用户名，类似Administrator
	static std::string GetSysUserName();
	//?获得本机中处理器的数量
	static int GetCPUCount();
	//?将字符串写入到剪贴板
	static void setClipboardData(HWND hWnd, const std::string& str);
	//?获取剪贴板内容
	static std::string GetClipboardData(HWND hWnd);
	//?获取环境变量值
	static std::string GetEnvironment(const char* name);
	//?创建目录
	static bool CreateDir(const std::string& dir);
	//?销毁目录
	static bool DestroyDir(const std::string& dir);
	//?检测目录或文件是否存在
	static bool DirOrFileAccess(const std::string& dir);
	//?获取控制台句柄
	static HWND GetConsoleHwnd();
	//?创建动态的二维数组，返回空代表失败，不为空则全部成功创建
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);
	//?销毁动态的二位数组，内部有判空
	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);

public:
    static bool ifRedirFrobid;
    static PVOID oldValue;
};

#include "CSystem.inl"