#pragma once
#include <windows.h>
#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include "CSystemMacro.h"

#define SafeDelete(ptr) \
if (ptr != nullptr)\
{\
	delete ptr;\
	ptr = nullptr;\
}

class CSystemAPI CSystem
{
public:
	//?实测CPU主频
	static double GetCPUSpeedGHz();
	//?获取系统任务栏矩形大小
	static RECT GetTaskbarRect();
	//?获取屏幕分辨率
	static RECT GetWindowResolution();
	//?返回屏幕右下角坐标，不包含任务栏
	static POINT taskbarRightBottomPoint();
	//?返回屏幕中央点
	static POINT screenCenterPoint();
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
    //?打印map
	static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");
    //?打印vector
	static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");
    //?清空scanf缓存区
    static void ClearScanf();
	//?获取执行参数列表
	static std::vector<std::string> exeParam();
	//?获取密码
	static std::string PasswordScanf();
	//?获取线程ID号
	static uint32_t SystemThreadId();
	//?获取CPU核数
	static int32_t GetCPUCoreCount();
	//?包含目录的拷贝，可以自动创建文件夹，同步函数
	static bool ShellCopy(const char* from, const char* dest);
	//?获取操作系统版本号
	static int32_t GetSystemVersionNum();
	//获取第一个和名字匹配的进程pid，填空表示获取本进程
	static int32_t processFirstPid(const std::string& processName = "");
	//?获取进程PID，耗时10毫秒左右，填空表示获取本进程
	static std::vector<int32_t> processPid(const std::string& processName = "");
	//?获取进程名，耗时10毫秒左右
	static std::string processName(int32_t pid);
	//?获取进程名，耗时10毫秒左右
	static std::wstring processNameW(int32_t pid);
	//?获取电脑名
	static std::string getComputerName();
	//返回本进程所在路径，带\符号
	static std::string GetCurrentExePath();
	//返回进程名，没有后缀名
	static std::string GetCurrentExeName();
	//获取文件相关信息，flag1表示获取文件全名，flag2表示获取文件后缀名，flag3表示获取文件名，flag4表示获取前面的路径带\\符号
	static std::string GetName(const std::string& path, int32_t flag);
	//删除文件
	static bool deleteFile(const char* path);
	//控制台输入输出
	static std::string inputString(const std::string& tip);
	//关闭进程
	static void killProcess(int32_t pid);
	//获取所有的命令行参数，第一个为文件路径
	static std::vector<std::string> mainParam();
	//重命名，传目录则重命名目录，传文件重命名文件，新文件或空目录若已存在则先删除，新目录不可以是老目录的子目录
	static bool rename(const std::string& oldPath, const std::string& newPath);
	//文件是否存在
	static bool fileExist(const std::string& filePath);
	//判断当前鼠标左键是否按下
	static bool isMouseLeftDown();
	//判断当前鼠标右键是否按下
	static bool isMouseRightDown();
	//判断当前鼠标中键是否按下
	static bool isMouseMidDown();
    //将QRect中4个点中小于0的点换成0
	static RECT rectValid(const RECT& rect);
	//time_t转化为字符串时间，如果不是本地时间就是格林威治时间
	static std::string timetToStr(time_t timet, bool isLocal = true);
	//获取common中的文件名路径
	static std::string commonFile(const std::string& name);
	//unicode转ansi
	static std::string UnicodeToAnsi(const std::wstring& wstrSrc);
	//ansi转unicode
	static std::wstring AnsiToUnicode(const std::string& strSrc);
	//读取文件
	std::string readFile(const std::string& path);

public:
    static bool ifRedirFrobid;
    static PVOID oldValue;
};

#include "CSystem.inl"