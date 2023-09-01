#pragma once
#ifdef _WIN32
#include <windows.h>
#endif
#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include "CSystemMacro.h"

class CSystemAPI CSystem
{
public:
#ifdef _WIN32
	//获取系统任务栏矩形大小
	static RECT GetTaskbarRect();
	//获取屏幕分辨率
	static RECT GetWindowResolution();
	//返回屏幕右下角坐标，不包含任务栏
	static POINT taskbarRightBottomPoint();
	//返回屏幕中央点
	static POINT screenCenterPoint();
	//将字符串写入到剪贴板
	static void setClipboardData(HWND hWnd, const std::string& str);
	//获取剪贴板内容
	static std::string GetClipboardData(HWND hWnd);
	//获取控制台句柄
	static HWND GetConsoleHwnd();
	//根据pid获取窗口句柄
	static HWND GetHwndByProcessId(uint32_t dwProcessId);
	//返回ocx所在路径，带\符号
	static std::string GetRegOcxPath(const std::string& classid);
	//禁用重定向，64位系统访问System32时会进入到SysWOW64中
	static void* ForbidRedir();
	//恢复重定向
	static void RecoveryRedir(void* oldValue);
	//获取去除任务栏及软件最大化时软件内任务栏后屏幕高度
	static int GetVisibleHeight();
	//外部打开网页
	static void OpenWebPage(const std::string& webPage);
	//判断当前鼠标左键是否按下
	static bool isMouseLeftDown();
	//判断当前鼠标右键是否按下
	static bool isMouseRightDown();
	//判断当前鼠标中键是否按下
	static bool isMouseMidDown();
	//包含目录的拷贝，可以自动创建文件夹，同步函数
	static bool ShellCopy(const char* from, const char* dest);
	//获取第一个和名字匹配的进程pid
	static uint32_t processFirstPid(const std::wstring& processNameW);
	//获取进程PID，耗时10毫秒左右
	static std::vector<uint32_t> processPid(const std::wstring& processNameW);
	//获取进程名，耗时10毫秒左右
	static std::wstring processNameW(uint32_t pid);
	//获取指定pid进程所有挂载的动态库，第一个固定为exe进程名，如果为空说明获取失败
	static std::vector<std::string> processMountDll(uint32_t pid);
	//获取所有摄像头名，内部会调用CoUninitialize
	static std::vector<std::string> allCameraName();
	//获取所有设备信息，左边为设备类型，右边为指定类型的所有设备信息
	static std::map<std::string, std::vector<std::string>> allDeviceInfo();
#endif
	//实测CPU主频，GHz位单位
	static double GetCPUSpeedGHz();
	//跨平台Sleep，毫秒
	static void Sleep(long long milliseconds);
	//生成UUID，传入1表示带-，传0表示不带，小写
	static std::string uuid(int flag = 1);
	//复制文件
	static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);
	//获取当前操作系统位数
	static int GetSystemBits();
	//获取当前系统用户名，类似Administrator
	static std::string GetSysUserName();
	//获取环境变量值
	static std::string GetEnvironment(const char* name);
	//创建目录，目录存在或者创建多级不存在的目录会返回false，带不带\都可以
	static bool CreateDir(const std::string& dir);
	//销毁目录，只能删除空文件夹，带不带\都可以
	static bool DestroyDir(const std::string& dir);
	//检测目录或文件是否存在，检测目录带不带\都可以，同一目录原本就不可能出现文件夹和文件重名的情况
	static bool DirOrFileExist(const std::string& dir);
	//打开系统文件夹
	static void OpenFolder(const std::string& folder);
	//打开系统文件夹并选择文件
	static void OpenFolderAndSelectFile(const std::string& file);
	//外部打开文件，linux下如果是调用外部程序打开需要加程序名，例如gedit
	static void OpenFile(const std::string& file);
	//创建动态的二维数组，返回空代表失败，不为空则全部成功创建
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);
	//销毁动态的二位数组，内部有判空
	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);
	//将RECT中4个点中小于0的点换成0
	template <typename TRECT>
	static TRECT rectValid(const TRECT& rect);
	//将QRect中4个点中小于0的点换成0
	template <typename TQRect>
	static TQRect qrectValid(const TQRect& rect);
	//打印map
	static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");
	//打印vector
	static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");
	//清空scanf缓存区
	static void ClearScanf();
	//获取执行参数列表，windows下不会读取参数，linux下会读取，第一个是可执行文件的绝对路径
	static std::vector<std::string> exeParam(int argc = 0, char** argv = nullptr);
	//获取密码
	static std::string PasswordScanf();
	//获取系统命令执行结果，如果是false则只能执行一句，如果是true可以|findstr，linux下频繁信号可能会导致该函数中的pclose后台卡死出现僵尸进程，该函数不会因此卡死
	static int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd = false);
	//获取线程ID号
	static uint32_t SystemThreadId();
	//获取CPU线程数，双核四线程即获取到4，四核四线程获取到4，和设备管理器中CPU数量一致
	//线程池数量如果是纯计算则直接使用线程数，如果含有数据库数据交互、文件上传下载、网络数据传输这些IO阻塞则乘以一个系数，一般是2
	static int32_t GetCPUCoreCount();
	//获取操作系统版本号，linux下为1604，1804...
	static int32_t GetSystemVersionNum();
	//获取本进程pid
	static uint32_t currentProcessPid();
	//获取第一个和名字匹配的进程pid，linux下为最后一个打开的该名字的进程
	static uint32_t processFirstPid(const std::string& processName);
	//获取进程PID，耗时10毫秒左右，linux下后打开的进程号在前面
	static std::vector<uint32_t> processPid(const std::string& processName);
	//获取进程全名，耗时10毫秒左右
	static std::string processName(uint32_t pid);
	//获取本进程所占内存，单位字节，linux下只准确到kb
	static uint64_t currentMemory();
	//获取时间
	static uint32_t GetTickCount();
	//获取高精度时间
	static std::chrono::high_resolution_clock::time_point GetHighTickCount();
	//获取执行时间，毫秒，包含int64_t到int32_t的强转
	static int32_t GetHighTickCountMilliRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取执行时间，微秒，包含int64_t到int32_t的强转
	static int32_t GetHighTickCountMicroRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取执行时间，纳秒
	static int64_t GetHighTickCountNanoRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取执行时间，毫秒，包含int64_t到int32_t的强转
	static int32_t GetMilliRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取执行时间，微秒，包含int64_t到int32_t的强转
	static int32_t GetMicroRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取执行时间，纳秒
	static int64_t GetNanoRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	//获取电脑名
	static std::string getComputerName();
	//获取动态库所在路径
	static std::string GetCurrentDllPath();
	//返回本进程所在路径，带\符号
	static std::string GetCurrentExePath();
	//返回进程名，没有后缀名
	static std::string GetCurrentExeName();
	//返回进程名，带后缀名
	static std::string GetCurrentExeFullName();
	//获取临时目录，带\符号
	static std::string GetSystemTempPath();
	//获取文件相关信息
	//flag1表示获取文件全名，flag2表示获取文件后缀名，如果文件名没有点，则后缀名为空
	//flag3表示获取文件名，flag4表示获取前面的路径带\\符号
	static std::string GetName(const std::string& path, int32_t flag);
	//删除文件，windows下只能删除文件不能删除文件夹，linux下可以删除文件或空文件夹，带不带\都可以
	static bool deleteFile(const char* path);
	//控制台带提示输入
	static std::string inputString(const std::string& tip);
	//关闭进程
	static void killProcess(int32_t pid);
	//重命名，传目录则重命名目录，传文件重命名文件，新文件或空目录若已存在则先删除，新目录不可以是老目录的子目录，新目录若不是空目录重命名不成功
	//新目录可以是不存在的目录，但不可以多级不存在，老目录内部可以有文件和文件夹，新文件可以不存在，老目录或老文件在重命名成功后会被删除
	static bool rename(const std::string& oldPath, const std::string& newPath);
	//time_t转化为字符串时间，如果不是本地时间就是格林威治时间
	static std::string timetToStr(time_t timet, bool isLocal = true);
	//获取common中的文件名路径
	static std::string commonFile(const std::string& name);
	//读取文件
	static std::string readFile(const std::string& path);
	//保存文件
	static void saveFile(const std::string& content, const std::string& path);
	/** 查找文件，内部没有禁止路径重定向
	@param [in] strPath 必须传文件夹路径，寻找文件夹以下，如果传空字符串则查找exe所在路径，带不带\都可以
	@param [in] flag 1表示查找文件，2表示查找文件后缀名，3表示查找所有文件，fileStr不起作用
	@param [in] fileStr 传文件名带后缀名或后缀名不带点，多个后缀名点分隔，如果填2传空或分隔后有空字符串则会搜索不含后缀名的文件，不查找文件夹
	@param [in] EveryFilePath 在每添加一个文件路径之前调用此函数，返回true表示中断搜索，继续搜索返回false
	@param [in] unVisitPath 把当前文件夹下不可访问的文件夹列出来，传空代表不存储，所有文件夹路径带\符号
	@return 返回所有查找到文件的绝对路径
	*/
	static std::vector<std::string> findFilePath(const std::string& strPath,
		int32_t flag = 3,
		const std::string& fileStr = "",
		const std::function<bool (const std::string&)>& EveryFilePath = nullptr,
		std::vector<std::string>* unVisitPath = nullptr);
};

#include "CSystem.inl"
