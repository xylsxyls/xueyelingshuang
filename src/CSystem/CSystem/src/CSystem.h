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
#include <new>
#include "CSystemMacro.h"

/** 系统工具集合，封装跨平台系统信息、文件目录、进程、时间、命令执行以及部分Windows专用能力
*/
class CSystemAPI CSystem
{
public:
#ifdef _WIN32
	/** 获取系统任务栏矩形大小
	@return 返回任务栏矩形，获取失败时字段通常为0
	*/
	static RECT GetTaskbarRect();
	/** 获取屏幕分辨率
	@return 返回屏幕矩形，right/bottom表示宽高边界
	*/
	static RECT GetWindowResolution();
	/** 返回屏幕右下角坐标，不包含任务栏
	@return 返回可见工作区右下角坐标
	*/
	static POINT taskbarRightBottomPoint();
	/** 返回屏幕中央点
	@return 返回主屏幕中心坐标
	*/
	static POINT screenCenterPoint();
	/** 将字符串写入到剪贴板
	@param [in] hWnd 打开剪贴板时使用的窗口句柄
	@param [in] str 要写入剪贴板的Ansi字符串
	*/
	static void setClipboardData(HWND hWnd, const std::string& str);
	/** 获取剪贴板内容
	@param [in] hWnd 打开剪贴板时使用的窗口句柄
	@return 返回剪贴板中的Ansi文本，没有文本时返回空字符串
	*/
	static std::string GetClipboardData(HWND hWnd);
	/** 获取控制台窗口句柄
	@return 返回当前进程控制台窗口句柄，获取失败返回NULL
	*/
	static HWND GetConsoleHwnd();
	/** 根据进程ID获取窗口句柄
	@param [in] dwProcessId 进程ID
	@return 返回匹配进程的窗口句柄，找不到返回NULL
	*/
	static HWND GetHwndByProcessId(uint32_t dwProcessId);
	/** 返回OCX注册路径
	@param [in] classid OCX的ClassId
	@return 返回OCX所在目录，带路径分隔符，失败返回空字符串
	*/
	static std::string GetRegOcxPath(const std::string& classid);
	/** 禁用WOW64文件系统重定向
	@return 返回恢复重定向时需要传回的旧值
	*/
	static void* ForbidRedir();
	/** 恢复WOW64文件系统重定向
	@param [in] oldValue ForbidRedir返回的旧值
	*/
	static void RecoveryRedir(void* oldValue);
	/** 获取去除任务栏后的可见屏幕高度
	@return 返回工作区可见高度
	*/
	static int GetVisibleHeight();
	/** 使用系统默认方式打开网页
	@param [in] webPage 网页URL
	*/
	static void OpenWebPage(const std::string& webPage);
	/** 判断当前鼠标左键是否按下
	@return 返回左键是否处于按下状态
	*/
	static bool isMouseLeftDown();
	/** 判断当前鼠标右键是否按下
	@return 返回右键是否处于按下状态
	*/
	static bool isMouseRightDown();
	/** 判断当前鼠标中键是否按下
	@return 返回中键是否处于按下状态
	*/
	static bool isMouseMidDown();
	/** 复制文件或目录，目标目录不存在时会自动创建
	@param [in] from 源路径
	@param [in] dest 目标路径
	@return 返回是否复制成功
	*/
	static bool ShellCopy(const char* from, const char* dest);
	/** 获取第一个和名字匹配的进程ID
	@param [in] processNameW 进程名宽字符串
	@return 返回进程ID，找不到返回0
	*/
	static uint32_t processFirstPid(const std::wstring& processNameW);
	/** 获取指定进程名的全部进程ID
	@param [in] processNameW 进程名宽字符串
	@return 返回进程ID列表
	*/
	static std::vector<uint32_t> processPid(const std::wstring& processNameW);
	/** 根据进程ID获取进程名
	@param [in] pid 进程ID
	@return 返回进程名宽字符串，失败返回空字符串
	*/
	static std::wstring processNameW(uint32_t pid);
	/** 获取指定进程挂载的动态库列表
	@param [in] pid 进程ID
	@return 返回动态库路径列表，第一个通常是exe路径，空列表表示失败
	*/
	static std::vector<std::string> processMountDll(uint32_t pid);
	/** 获取所有摄像头名称
	@return 返回摄像头名称列表，内部会调用CoUninitialize
	*/
	static std::vector<std::string> allCameraName();
	/** 获取所有设备信息
	@return 返回设备类型到设备信息列表的映射
	*/
	static std::map<std::string, std::vector<std::string>> allDeviceInfo();
#endif
	/** 实测CPU主频
	@return 返回CPU主频，单位GHz
	*/
	static double GetCPUSpeedGHz();
	/** 跨平台线程休眠
	@param [in] milliseconds 休眠时间，单位毫秒
	*/
	static void Sleep(long long milliseconds);
	/** 生成UUID字符串
	@param [in] flag 1表示带横线，0表示不带横线
	@return 返回小写UUID字符串
	*/
	static std::string uuid(int flag = 1);
	/** 复制文件
	@param [in] dstFile 目标文件路径
	@param [in] srcFile 源文件路径
	@param [in] over 目标存在时是否覆盖
	*/
	static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);
	/** 获取当前操作系统位数
	@return 返回32或64，无法判断时返回0
	*/
	static int GetSystemBits();
	/** 获取当前系统用户名
	@return 返回当前登录用户名，例如Administrator
	*/
	static std::string GetSysUserName();
	/** 获取环境变量值
	@param [in] name 环境变量名
	@return 返回环境变量值，不存在返回空字符串
	*/
	static std::string GetEnvironment(const char* name);
	/** 创建目录
	@param [in] dir 目录路径，带不带路径分隔符均可
	@return 返回是否创建成功，目录已存在或多级父目录不存在时返回false
	*/
	static bool CreateDir(const std::string& dir);
	/** 删除空目录
	@param [in] dir 目录路径，带不带路径分隔符均可
	@return 返回是否删除成功
	*/
	static bool DestroyDir(const std::string& dir);
	/** 检测目录或文件是否存在
	@param [in] dir 目录或文件路径，目录带不带路径分隔符均可
	@return 返回路径是否存在
	*/
	static bool DirOrFileExist(const std::string& dir);
	/** 打开系统文件夹
	@param [in] folder 文件夹路径
	*/
	static void OpenFolder(const std::string& folder);
	/** 打开系统文件夹并选中文件
	@param [in] file 文件路径
	*/
	static void OpenFolderAndSelectFile(const std::string& file);
	/** 使用系统或指定外部程序打开文件
	@param [in] file 文件路径，Linux下可包含外部程序名，例如gedit
	*/
	static void OpenFile(const std::string& file);
	/** 退出fork创建的子进程，不执行父进程继承来的全局析构和atexit逻辑
	@param [in] exitCode 子进程退出码
	*/
	static void childProcessExit(int32_t exitCode);
	/** 创建动态二维数组
	@param [in] row 行数
	@param [in] column 列数
	@return 返回二维数组指针，失败返回空指针
	*/
	template <typename TypeClass>
	static TypeClass** CreateDyadicArray(int32_t row, int32_t column);
	/** 销毁动态二维数组
	@param [in] classPtr CreateDyadicArray返回的二维数组指针
	@param [in] row 行数
	*/
	template <typename TypeClass>
	static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);
	/** 将RECT风格对象中小于0的点修正为0
	@param [in] rect 原矩形对象，要求有left/top/right/bottom成员
	@return 返回修正后的矩形对象
	*/
	template <typename TRECT>
	static TRECT rectValid(const TRECT& rect);
	/** 将QRect风格对象中小于0的点修正为0
	@param [in] rect 原矩形对象，要求有left/top/right/bottom访问器和set函数
	@return 返回修正后的矩形对象
	*/
	template <typename TQRect>
	static TQRect qrectValid(const TQRect& rect);
	/** 输出字符串map到控制台或文件
	@param [in] stringMap 要输出的map
	@param [in] path 输出文件路径，空字符串表示输出到控制台
	*/
	static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");
	/** 输出字符串vector到控制台或文件
	@param [in] stringVector 要输出的vector
	@param [in] path 输出文件路径，空字符串表示输出到控制台
	*/
	static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");
	/** 清空scanf输入缓存区
	*/
	static void ClearScanf();
	/** 获取执行参数列表
	@param [in] argc main函数参数数量，Windows下可传默认值
	@param [in] argv main函数参数数组，Windows下可传默认值
	@return Windows下返回进程路径，Linux下返回argv列表，第一个通常是可执行文件绝对路径
	*/
	static std::vector<std::string> exeParam(int argc = 0, char** argv = nullptr);
	/** 从控制台读取密码
	@return 返回用户输入的密码文本
	*/
	static std::string PasswordScanf();
	/** 执行系统命令并获取输出
	@param [in] command 命令文本
	@param [in] result 输出命令执行结果
	@param [in] isShowCmd 是否通过系统shell展示执行，true时可使用管道等shell能力
	@return 返回系统命令退出码或内部错误码
	*/
	static int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd = false);
	/** 获取当前线程ID
	@return 返回当前系统线程ID
	*/
	static uint32_t SystemThreadId();
	/** 获取CPU硬件线程数
	@return 返回系统可用CPU线程数，适合作为纯计算线程池的默认规模
	*/
	static int32_t GetCPUCoreCount();
	/** 获取操作系统版本号
	@return Windows返回版本编码，Linux返回类似1604、1804的发行版版本号
	*/
	static int32_t GetSystemVersionNum();
	/** 获取当前进程ID
	@return 返回当前进程ID
	*/
	static uint32_t currentProcessPid();
	/** 获取第一个和名字匹配的进程ID
	@param [in] processName 进程名
	@return 返回进程ID，Linux下通常为最后打开的同名进程，找不到返回0
	*/
	static uint32_t processFirstPid(const std::string& processName);
	/** 获取指定进程名的全部进程ID
	@param [in] processName 进程名
	@return 返回进程ID列表，Linux下后打开的进程号在前面
	*/
	static std::vector<uint32_t> processPid(const std::string& processName);
	/** 根据进程ID获取进程全名
	@param [in] pid 进程ID
	@return 返回进程名，失败返回空字符串
	*/
	static std::string processName(uint32_t pid);
	/** 获取当前进程占用内存
	@return 返回内存字节数，Linux下精度通常到KB
	*/
	static uint64_t currentMemory();
	/** 获取系统运行毫秒计数
	@return 返回毫秒计数，可能按系统API语义回绕
	*/
	static uint32_t GetTickCount();
	/** 获取高精度时间点
	@return 返回当前高精度时钟时间点
	*/
	static std::chrono::high_resolution_clock::time_point GetHighTickCount();
	/** 获取从开始时间到现在的执行耗时，单位毫秒
	@param [in] beginTime 开始时间点
	@return 返回耗时毫秒数，内部会从int64_t强转为int32_t
	*/
	static int32_t GetHighTickCountMilliRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取从开始时间到现在的执行耗时，单位微秒
	@param [in] beginTime 开始时间点
	@return 返回耗时微秒数，内部会从int64_t强转为int32_t
	*/
	static int32_t GetHighTickCountMicroRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取从开始时间到现在的执行耗时，单位纳秒
	@param [in] beginTime 开始时间点
	@return 返回耗时纳秒数
	*/
	static int64_t GetHighTickCountNanoRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取两个时间点之间的耗时，单位毫秒
	@param [in] endTime 结束时间点
	@param [in] beginTime 开始时间点
	@return 返回耗时毫秒数，内部会从int64_t强转为int32_t
	*/
	static int32_t GetMilliRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取两个时间点之间的耗时，单位微秒
	@param [in] endTime 结束时间点
	@param [in] beginTime 开始时间点
	@return 返回耗时微秒数，内部会从int64_t强转为int32_t
	*/
	static int32_t GetMicroRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取两个时间点之间的耗时，单位纳秒
	@param [in] endTime 结束时间点
	@param [in] beginTime 开始时间点
	@return 返回耗时纳秒数
	*/
	static int64_t GetNanoRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);
	/** 获取电脑名
	@return 返回当前主机名，失败返回空字符串
	*/
	static std::string getComputerName();
	/** 获取当前动态库所在路径
	@return 返回动态库目录，通常带路径分隔符
	*/
	static std::string GetCurrentDllPath();
	/** 获取当前进程所在目录
	@return 返回exe所在目录，带路径分隔符
	*/
	static std::string GetCurrentExePath();
	/** 获取当前进程名，不带后缀名
	@return 返回进程名
	*/
	static std::string GetCurrentExeName();
	/** 获取当前进程完整文件名
	@return 返回进程文件名，带后缀名
	*/
	static std::string GetCurrentExeFullName();
	/** 获取系统临时目录
	@return 返回临时目录，带路径分隔符
	*/
	static std::string GetSystemTempPath();
	/** 获取路径中的文件信息
	@param [in] path 文件或目录路径
	@param [in] flag 1文件全名，2后缀名，3文件名，4前置路径并带路径分隔符
	@return 返回指定部分字符串，无法解析时返回空字符串
	*/
	static std::string GetName(const std::string& path, int32_t flag);
	/** 删除文件或空目录
	@param [in] path 文件或目录路径，Windows下只删除文件，Linux下可删除文件或空目录
	@return 返回是否删除成功
	*/
	static bool deleteFile(const char* path);
	/** 控制台带提示读取一行字符串
	@param [in] tip 输入提示文本
	@return 返回用户输入内容
	*/
	static std::string inputString(const std::string& tip);
	/** 关闭进程
	@param [in] pid 进程ID
	*/
	static void killProcess(int32_t pid);
	/** 重命名文件或目录
	@param [in] oldPath 原文件或目录路径
	@param [in] newPath 新文件或目录路径，已存在的新文件或空目录会先删除
	@return 返回是否重命名成功
	*/
	static bool rename(const std::string& oldPath, const std::string& newPath);
	/** 将time_t转换为时间字符串
	@param [in] timet 时间值
	@param [in] isLocal 是否按本地时间转换，false表示格林威治时间
	@return 返回格式化后的时间字符串
	*/
	static std::string timetToStr(time_t timet, bool isLocal = true);
	/** 获取common目录中的文件路径
	@param [in] name common目录下的文件名
	@return 返回文件完整路径
	*/
	static std::string commonFile(const std::string& name);
	/** 读取整个文件内容
	@param [in] path 文件路径
	@return 返回文件内容，失败返回空字符串
	*/
	static std::string readFile(const std::string& path);
	/** 保存文件内容
	@param [in] content 要写入的内容
	@param [in] path 文件路径
	*/
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