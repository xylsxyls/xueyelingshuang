#pragma once
#include "CSystemMacro.h"
// Wasm只提供时间、路径字节串、参数解析及纯模板工具；其余原生系统能力须条件调用
#if defined(__EMSCRIPTEN__)
#define CSYSTEM_HAS_NATIVE_API 0
#else
#define CSYSTEM_HAS_NATIVE_API 1
#endif

#include <string>
#include <stdint.h>
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include <new>
#include <atomic>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
struct IEnumMoniker;
#endif
#ifdef __APPLE__
#include <TargetConditionals.h>
#endif
#if !defined(__EMSCRIPTEN__) && (defined(__ANDROID__) || (defined(__APPLE__) && TARGET_OS_IPHONE) || defined(PROCESS_WORK_PORTABLE_IDENTITY))
#define CSYSTEM_SHARED_IDENTITY 1
#endif

// 桌面交互、控制台输入和整机租约不在Android/iOS/Wasm公开
#if CSYSTEM_HAS_NATIVE_API && !defined(__ANDROID__) && (!defined(__APPLE__) || !TARGET_OS_IPHONE)
#define CSYSTEM_HAS_DESKTOP_API 1
#else
#define CSYSTEM_HAS_DESKTOP_API 0
#endif

/** 系统工具集合，只依赖标准库和系统接口，封装系统信息、文件目录、进程、时间及平台专属能力
所有函数均为静态成员；移动端身份登记目录由宿主在启动期配置，其他生命周期由各接口明确约定
*/
class CSystemAPI CSystem
{
public:

#if CSYSTEM_HAS_NATIVE_API
    /** 配置移动端身份登记专用目录，不借用其他库的配置
    Android使用应用内部专用目录，iOS使用App Group容器中的专用目录
    @param [in] directory 已存在且属于当前UID的绝对路径；首次登记前串行配置，各参与进程使用同一路径
    @return 配置成功true；空值、非法目录或首次登记尝试后更换路径为false；桌面原生后端只接受空路径
    */
    static bool setProcessDirectory(const std::string& directory);

#if CSYSTEM_HAS_DESKTOP_API
    /** 取得桌面整机命名进程租约；内核锁在崩溃时自动释放
    @param [in] name ASCII角色名，所有安装目录和版本使用同一个名称
    @param [in] replace 已占用时请求旧进程正常退出并等待
    @param [in] timeoutMs 替换等待上限，最多30000毫秒
    @param [out] state 不透明租约；成功后由同一线程释放
    @param [in] isolated 测试宿主隔离键；生产调用必须为空
    @return 1取得，0已有实例应复用，-1系统失败或替换超时
    */
    static int32_t acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
        std::vector<uint64_t>& state, const std::string& isolated = std::string());
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 发布或撤销本进程可接入状态，只有完整初始化后才置true
    @param [in] state 已取得租约
    @param [in] ready 当前是否可接入
    @return 系统操作是否成功
    */
    static bool setProcessLeaseReady(const std::vector<uint64_t>& state, bool ready);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 查询替换请求，由主循环转换为正常关闭
    @param [in] state 已取得租约
    @return 是否已收到停止请求；无效状态返回true
    */
    static bool processLeaseStopRequested(const std::vector<uint64_t>& state);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 撤销就绪并释放内核句柄；只能在所有工作已结束后调用
    @param [in,out] state 成功取得的租约，返回时清空
    */
    static void releaseProcessLease(std::vector<uint64_t>& state);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 验证指定PID已发布就绪；实际接入仍须完成IPC和TLS握手
    @param [in] name 角色名
    @param [in] pid 正进程号
    @return 当前已就绪；启动中、停止中及不存在均为false
    */
    static bool isProcessLeaseReady(const std::string& name, uint32_t pid);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 设置当前用户的Windows登录启动项，仅Windows公开
    @param [in] name ASCII启动项名，只允许字母、数字、点、下划线和连字符，1至80字节
    @param [in] command UTF8完整命令行，Windows最多260个宽字符；空串只移除指定启动项
    @return 0成功或系统错误码；不创建服务或启动进程
    */
    static uint32_t setUserLoginStartup(const std::string& name, const std::string& command);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 以独立后台进程启动绝对路径，不经shell，仅Windows公开
    @param [in] executable UTF8绝对路径
    @param [in] arguments 原始UTF8参数，各自引用转义，最多64项
    @param [out] pid 成功子进程号，失败0
    @param [out] monitor 成功取得的独占监视句柄，须closeProcessMonitor；不结束子进程
    @return 0或系统错误码，不继承宿主句柄
    */
    static uint32_t startDetachedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        uint32_t& pid, uint64_t& monitor);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 后台启动进程并独占创建输出文件，不经shell，仅Windows公开
    @param [in] executable UTF8绝对EXE路径
    @param [in] arguments 逐项原始参数，最多64项
    @param [in] output UTF8绝对日志路径，必须不存在；只继承该文件和NUL输入
    @param [out] pid 成功进程号，失败0
    @param [out] monitor 独占监视句柄，由closeProcessMonitor释放
    @return 0或系统错误码；不会覆盖已有日志，不结束其他进程
    */
    static uint32_t startCapturedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& output, uint32_t& pid, uint64_t& monitor);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 采样有效监视句柄所属进程资源，仅Windows公开
    @param [in] monitor 0为当前进程，其他为本库取得且未释放的句柄，需具备查询和读取权限
    @param [out] privateBytes 私有提交字节
    @param [out] workingSet 当前工作集字节
    @param [out] cpuTime100ns 内核和用户CPU时间之和，单位100纳秒
    @param [out] handles 进程句柄数
    @param [out] threads 进程线程数
    @return 完整采样成功true；失败各输出清0，不把无权限当已退出
    */
    static bool queryProcessResources(uint64_t monitor, uint64_t& privateBytes, uint64_t& workingSet,
        uint64_t& cpuTime100ns, uint32_t& handles, uint32_t& threads);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** Windows打开进程退出监视句柄，持有期间不会混淆复用PID
    @param [in] pid 目标进程号
    @return 独占句柄，失败为0；权限不足不当成已退出
    */
    static uint64_t openProcessMonitor(uint32_t pid);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 非阻塞查询已持有进程的退出事实
    @param [in] monitor 有效句柄
    @param [out] exitCode 已退出时为实际退出码，其他情况0
    @return 1已退出，0仍运行，-1错误
    */
    static int32_t pollProcessMonitor(uint64_t monitor, int32_t& exitCode);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 关闭监视句柄，不关闭目标进程
    @param [in,out] monitor 本模块创建的独占句柄，释放后0
    */
    static void closeProcessMonitor(uint64_t& monitor);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** Windows无控制台程序将CRT输出绑定到调用方明确重定向的管道/文件
    @return 绑定成功；没有重定向时为true，不分配或附加控制台
    */
    static bool bindInheritedOutput();
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif // CSYSTEM_NATIVE_API

#ifdef _WIN32

#if CSYSTEM_HAS_NATIVE_API
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

    /** 查找指定进程的可见顶层窗口，不改变原GetHwndByProcessId的筛选语义
    @param [in] processId 进程ID，0不匹配
    @return 首个匹配的可见窗口，未找到返回nullptr；仅Windows提供
    */
    static HWND findVisibleWindowByProcessId(uint32_t processId);

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

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取第一个和名字匹配的进程ID
    @param [in] processNameW 进程名宽字符串
    @return 返回进程ID，找不到返回0
    */
    static uint32_t processFirstPid(const std::wstring& processNameW);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取指定进程名的全部进程ID
    @param [in] processNameW 进程名宽字符串
    @return 返回进程ID列表
    */
    static std::vector<uint32_t> processPid(const std::wstring& processNameW);
#endif // CSYSTEM_PLATFORM_CAPABILITY

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
#endif // CSYSTEM_NATIVE_API

#endif

#if CSYSTEM_HAS_NATIVE_API
#if CSYSTEM_HAS_DESKTOP_API
    /** 实测桌面CPU主频
    @return 返回CPU主频，单位GHz；沿用旧接口，64位Windows或测量失败返回0
    */
    static double GetCPUSpeedGHz();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 跨平台线程休眠
    @param [in] milliseconds 休眠时间，单位毫秒
    */
    static void Sleep(long long milliseconds);

    /** 网络轮询的短时阻塞等待，不占用CPU自旋或更改全局定时器分辨率
    @param [in] milliseconds 等待毫秒；Windows支持时使用高精度等待计时器，其他情况回退Sleep
    */
    static void sleepPrecise(uint32_t milliseconds);

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

    /** 判断普通文件是否存在，支持Windows宽字符路径
    @param [in] path 文件路径
    @return 文件存在且不是目录时返回true，否则返回false
    */
    static bool fileExists(const std::wstring& path);

#if CSYSTEM_HAS_DESKTOP_API
    /** 打开系统文件夹
    @param [in] folder 文件夹路径
    */
    static void OpenFolder(const std::string& folder);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 打开系统文件夹并选中文件
    @param [in] file 文件路径
    */
    static void OpenFolderAndSelectFile(const std::string& file);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if CSYSTEM_HAS_DESKTOP_API
    /** 使用系统或指定外部程序打开文件
    @param [in] file 文件路径，Linux下可包含外部程序名，例如gedit
    */
    static void OpenFile(const std::string& file);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 退出fork创建的子进程，不执行父进程继承来的全局析构和atexit逻辑
    @param [in] exitCode 子进程退出码
    */
    static void childProcessExit(int32_t exitCode);
#endif // CSYSTEM_NATIVE_API

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

#if CSYSTEM_HAS_NATIVE_API
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

#if CSYSTEM_HAS_DESKTOP_API
    /** 清空scanf输入缓存区
    */
    static void ClearScanf();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取执行参数列表
    @param [in] argc main函数参数数量，Windows下可传默认值
    @param [in] argv main函数参数数组，Windows下可传默认值
    @return Windows下返回进程路径，Linux下返回argv列表，第一个通常是可执行文件绝对路径
    */
    static std::vector<std::string> exeParam(int argc = 0, char** argv = nullptr);

#if CSYSTEM_HAS_DESKTOP_API
    /** 从控制台读取密码
    @return 返回用户输入的密码文本
    */
    static std::string PasswordScanf();
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 执行系统命令并获取输出
    @param [in] command 命令文本
    @param [in] result 输出命令执行结果
    @param [in] isShowCmd 是否通过系统shell展示执行，true时可使用管道等shell能力
    @return 返回系统命令退出码或内部错误码
    */
    static int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd = false);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取当前线程ID
    @return 返回当前系统线程ID
    */
    static uint32_t SystemThreadId();

    /** 获取CPU硬件线程数
    @return 返回系统可用CPU线程数，适合作为纯计算线程池的默认规模
    */
    static int32_t GetCPUCoreCount();

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    /** 获取Windows或Linux桌面的版本编号
    @return Windows返回版本编码，Linux返回类似1604、1804的发行版版本号
    */
    static int32_t GetSystemVersionNum();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取当前进程ID
    @return 返回当前进程ID
    */
    static uint32_t currentProcessPid();

    /** 仅在操作系统明确证明进程已退出/不存在时返回true，不把权限不足当成死亡
    @param [in] processId 正进程号；0或不可表示的值返回false
    @return 明确退出为true，存活或无法判定为false；PID不构成通信身份认证
    */
    static bool processDefinitelyExited(uint32_t processId);

    /** 查询进程实例身份。桌面使用系统创建时间；移动端使用同一共享目录中的参与者随机登记
    移动端首次调用前须配置CSystem::setProcessDirectory指定的目录；只认识已初始化本库的参与进程。
    @param [in] processId 正PID
    @param [out] identity 完整实例标识；失败置0，不代表目标已死亡
    @return 身份已知为true；权限不足、未登记或底层失败为false
    */
    static bool processIdentity(uint32_t processId, uint64_t& identity);
#endif // CSYSTEM_NATIVE_API

    /** 读取进程命令行参数为UTF8，不包含程序名；Windows不依赖ANSI argv
    @param [in] argc 标准main参数个数1至4096；Windows GUI入口可用0并传空argv
    @param [in] argv 标准main参数；Windows使用系统宽字符命令行，GUI入口允许与argc=0成对为空
    @param [out] arguments 成功替换列表，参数错误或编码失败保持原值
    @return 是否成功，标准库分配异常由调用边界处理
    */
    static bool commandArguments(int32_t argc, char** argv, std::vector<std::string>& arguments);

#if CSYSTEM_HAS_NATIVE_API
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取第一个和名字匹配的进程ID
    @param [in] processName 进程名
    @return 返回进程ID，Linux下通常为最后打开的同名进程，找不到返回0
    */
    static uint32_t processFirstPid(const std::string& processName);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取指定进程名的全部进程ID
    @param [in] processName 进程名
    @return 返回进程ID列表，Linux下后打开的进程号在前面
    */
    static std::vector<uint32_t> processPid(const std::string& processName);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 根据进程ID获取进程全名
    @param [in] pid 进程ID
    @return 返回进程名，失败返回空字符串
    */
    static std::string processName(uint32_t pid);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32) || defined(__linux__)
    /** 获取当前进程占用内存
    @return 返回内存字节数，Linux下精度通常到KB
    */
    static uint64_t currentMemory();
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif // CSYSTEM_NATIVE_API

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

#if CSYSTEM_HAS_NATIVE_API
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

    /** 取得当前可执行文件完整UTF8路径，不改变GetCurrentExePath的历史编码契约
    @return 完整路径，失败空；不依赖当前工作目录
    */
    static std::string currentExecutablePathUtf8();

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
#endif // CSYSTEM_NATIVE_API

    /** 获取路径中的文件信息
    @param [in] path 文件或目录路径
    @param [in] flag 1文件全名，2后缀名，3文件名，4前置路径并带路径分隔符
    @return 返回指定部分字符串，无法解析时返回空字符串
    */
    static std::string GetName(const std::string& path, int32_t flag);

    /** 拼接目录和相对文件名，只处理末尾分隔符，不访问文件系统或解析..路径
    @param [in] directory 目录，可空；支持正斜杠和反斜杠，已有末尾分隔符不重复添加
    @param [in] name 相对文件名；调用方负责拒绝绝对路径和不允许的子目录
    @return 拼接路径；任一参数为空时返回另一参数
    */
    static std::string joinPath(const std::string& directory, const std::string& name);

#if CSYSTEM_HAS_NATIVE_API
    /** 规范绝对路径，可按要求解析实际文件和目录链接
    @param [in] path 非空路径，不允许零字符
    @param [in] requireExisting 为true时解析实际存在目标，为false时规范词法绝对路径
    @return 规范路径；失败返回空，Windows比较用路径统一ASCII大小写
    */
    static std::string canonicalPath(const std::string& path, bool requireExisting = true);

    /** 明确UTF8输入/输出的绝对路径；旧canonicalPath及GetCurrentExePath的历史编码契约保持
    @param [in] path UTF8非空路径，不允许零字符
    @param [in] requireExisting true解析实际目标，false规范绝对词法路径
    @return UTF8绝对路径，失败空；Windows统一ASCII大小写
    */
    static std::string canonicalPathUtf8(const std::string& path, bool requireExisting = true);

    /** 按UTF8路径创建普通目录及父目录，拒绝最终链接/重解析点
    @param [in] path 非空UTF8路径
    @return 完整普通目录已存在或成功创建
    */
    static bool createDirectoriesUtf8(const std::string& path);

    /** 判断路径是否严格位于目录内，存在目标会解析链接
    @param [in] path 待核对路径
    @param [in] directory 允许的根目录
    @param [in] requireExisting 是否要求目标实际存在
    @return 存在有效规范路径且严格包含时返回true，目录自身不算子项
    */
    static bool isPathWithin(const std::string& path, const std::string& directory, bool requireExisting = true);

    /** 将静止目录迁到不重叠目录，逐文件核对内容后才删除源
    @param [in] source 源目录；不存在视为已完成，拒绝符号链接和重解析点
    @param [in] destination 目标目录；同名文件不同内容时不覆盖
    @param [out] errorText 失败原因；失败保留尚未验证的源数据，可幂等重试
    @return 全部文件完成且源空目录移除时返回true；同步IO不应在交互线程运行
    */
    static bool migrateDirectory(const std::string& source, const std::string& destination, std::string& errorText);

    /** 创建普通目录及缺失父目录，已有普通目录视为成功
    @param [in] path 待创建的非空目录路径，拒绝目标重解析点或符号链接
    @return 完整目录存在且为普通目录返回true
    */
    static bool createDirectories(const std::string& path);
#endif // CSYSTEM_NATIVE_API

    /** 保证文件扩展名，匹配已有后缀时规范其大小写，否则追加后缀
    @param [in] path 原路径，空路径保持空
    @param [in] extension 含点的目标ASCII扩展名，例如.pdf；空扩展名保持原路径
    @return 新路径，不访问文件系统；ASCII大小写不敏感，保留路径其他字节
    */
    static std::string ensureFileExtension(const std::string& path, const std::string& extension);

#if CSYSTEM_HAS_NATIVE_API
    /** 删除文件或空目录
    @param [in] path 文件或目录路径，Windows下只删除文件，Linux下可删除文件或空目录
    @return 返回是否删除成功
    */
    static bool deleteFile(const char* path);

    /** 打开二进制输出文件，支持Windows宽字符路径
    @param [in] path 输出文件路径
    @param [out] errorText 打开失败原因，可为空
    @return 打开成功返回FILE指针，失败返回nullptr
    */
    static FILE* openBinaryOutputFile(const std::wstring& path, std::string* errorText = nullptr);

    /** 排他创建仅当前用户、系统和管理员可访问的私密文件，已存在时不覆盖
    @param [in] path 完整临时路径；宿主负责在可信目录内随机命名和最终替换
    @param [out] errorText 固定失败文案，不包含私密文件内容，可空
    @return 由CSystem关闭的FILE；Windows保护DACL，Unix权限0600且不跟随符号链接
    */
    static FILE* openPrivateBinaryOutputFile(const std::wstring& path, std::string* errorText = nullptr);

    /** 将已写数据刷新到操作系统持久介质，之后仍须检查close和最终原子替换
    @param [in] file 由CSystem创建且尚未关闭的输出文件
    @return fflush及系统提交成功
    */
    static bool flushBinaryOutputFile(FILE* file);

    /** 将已完成的同目录临时文件排他安装，目标存在时保持原目标不变
    @param [in] sourcePath 已提交并关闭的临时文件
    @param [in] destinationPath 正式路径
    @return 成功安装；失败时源可能仍存在，宿主只能清理自己创建的临时文件
    */
    static bool installFileIfAbsent(const std::wstring& sourcePath, const std::wstring& destinationPath);

    /** 按UTF8检查普通文件，拒绝目录、符号链接及Windows重解析点，保持旧plainFile编码契约
    @param [in] path 非空UTF8路径，不允许NUL
    @return 普通文件存在
    */
    static bool plainFileUtf8(const std::string& path);

    /** 读取普通文件的字节数与修改标记，拒绝符号链接和重解析点
    @param [in] path 完整UTF8路径
    @param [out] bytes 不可空，失败清零
    @param [out] stamp 不可空，仅用于比较修改前后，不作为跨平台UTC时间
    @return 成功返回true
    */
    static bool fileStampUtf8(const std::string& path, uint64_t* bytes, uint64_t* stamp);

    /** 写入由 openBinaryOutputFile 创建的文件，避免跨 DLL 使用不同 CRT 的 FILE
    @param [in] file CSystem 创建的输出文件
    @param [in] data 待写字节
    @param [in] size 字节数量
    @return 完整写入返回 true
    */
    static bool writeBinaryOutputFile(FILE* file, const void* data, size_t size);

    /** 关闭 CSystem 创建的输出文件并检查缓冲写入结果
    @param [in] file 输出文件，关闭后不再有效
    @return 刷新及关闭成功返回 true
    */
    static bool closeBinaryOutputFile(FILE* file);

    /** 删除文件或空目录，支持Windows宽字符路径
    @param [in] path 文件或目录路径，Windows下只删除文件，Linux下可删除文件或空目录
    @return 返回是否删除成功
    */
    static bool deleteFile(const std::wstring& path);

#if CSYSTEM_HAS_DESKTOP_API
    /** 控制台带提示读取一行字符串
    @param [in] tip 输入提示文本
    @return 返回用户输入内容
    */
    static std::string inputString(const std::string& tip);
#endif // CSYSTEM_PLATFORM_CAPABILITY

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

    /** 将同文件系统临时文件替换目标文件，不预先删除目标
    @param [in] sourcePath 已写完并关闭的临时文件，失败时保留
    @param [in] destinationPath 目标宽字符路径，允许已存在
    @return 替换成功返回 true；失败保留原目标
    */
    static bool replaceFile(const std::wstring& sourcePath, const std::wstring& destinationPath);
#endif // CSYSTEM_NATIVE_API

    /** 将time_t转换为时间字符串
    @param [in] timet 时间值
    @param [in] isLocal 是否按本地时间转换，false表示格林威治时间
    @return 返回格式化后的时间字符串
    */
    static std::string timetToStr(time_t timet, bool isLocal = true);

#if CSYSTEM_HAS_NATIVE_API
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

    /** 按预算读取二进制文件，逐块限制内存，不依赖预先查询的文件大小
    @param [in] path Unicode路径，不能为空或含零字符
    @param [in] maximumBytes 允许读取的最大字节数，超出即失败
    @param [out] data 成功替换结果，打开、读取、超限或关闭失败保留原值
    @return 完整读取且关闭成功返回true；分配异常传播但文件会关闭；同步IO须在合适的执行线程调用
    */
    static bool readBinaryFile(const std::wstring& path, size_t maximumBytes, std::vector<uint8_t>& data);

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

    /** 按UTF8路径打开可寻址普通文件，拒绝末级链接/重解析点
    @param [in] path 非空UTF8路径，最长4096
    @param [in] mode 1既有只读，2独占新建读写不覆盖，3既有读写不截断
    @param [out] error 可空，Win32错误或errno，成功0
    @return 独占句柄，失败-1；调用者负责closeFileHandle，不在网络/GUI执行者做慢I/O
    */
    static intptr_t openFileHandle(const char* path, uint32_t mode, uint32_t* error = nullptr);

    /** 独占创建可由其他执行者只读观察的输出文件，不覆盖既有文件
    @param [in] path UTF8绝对路径
    @param [out] error 可空系统错误
    @return 本模块文件句柄或-1；使用既有write/flush/closeFileHandle，其他写者不能打开
    */
    static intptr_t createObservedOutputFile(const char* path, uint32_t* error = nullptr);

    /** 复制普通文件句柄，原件仍归宿主；Windows副本共享文件位置，须同一执行者使用
    @param [in] native Windows HANDLE的intptr_t，其余平台fd
    @param [out] error 可空系统错误
    @return 新独占句柄，失败-1
    */
    static intptr_t duplicateFileHandle(intptr_t native, uint32_t* error = nullptr);

    /** 指定位置有界读取，EOF可少于capacity
    @param [in] handle 普通文件句柄
    @param [in] offset 字节位置，offset+capacity不超过INT64_MAX
    @param [out] output capacity非零时不可空
    @param [in] capacity 最大16MiB
    @param [out] received 不可空，成功字节数，失败0
    @param [out] error 可空系统错误
    @return 是否读取成功
    */
    static bool readFileHandle(intptr_t handle, uint64_t offset, uint8_t* output, uint32_t capacity, uint32_t* received, uint32_t* error = nullptr);

    /** 完整位置写；失败可能已写前缀，写完不等于持久化
    @param [in] handle 可写普通文件句柄
    @param [in] offset 位置，offset+size不超过INT64_MAX
    @param [in] input size非零时不可空
    @param [in] size 最大16MiB
    @param [out] error 可空系统错误
    @return 是否完整写入
    */
    static bool writeFileHandle(intptr_t handle, uint64_t offset, const uint8_t* input, uint32_t size, uint32_t* error = nullptr);

    /** 将已写文件数据持久化，失败不得提前确认/发布
    @param [in] handle 可写句柄
    @param [out] error 可空系统错误
    @return 是否持久化成功
    */
    static bool flushFileHandle(intptr_t handle, uint32_t* error = nullptr);

    /** 查询实际文件长度
    @param [in] handle 句柄
    @param [out] bytes 不可空，失败0
    @param [out] error 可空系统错误
    @return 是否成功
    */
    static bool fileHandleSize(intptr_t handle, uint64_t* bytes, uint32_t* error = nullptr);

    /** 设置文件长度，截断未确认尾部不代表持久化
    @param [in] handle 可写句柄
    @param [in] bytes 不超过INT64_MAX
    @param [out] error 可空系统错误
    @return 是否成功
    */
    static bool resizeFileHandle(intptr_t handle, uint64_t bytes, uint32_t* error = nullptr);

    /** 关闭并将调用者句柄置-1；-1重复关闭成功，不隐式flush
    @param [in,out] handle 不可空，关闭后不得再用旧值
    @param [out] error 可空系统错误
    @return 是否关闭成功
    */
    static bool closeFileHandle(intptr_t* handle, uint32_t* error = nullptr);

    /** 同文件系统发布已经持久化且关闭的普通临时文件
    @param [in] source UTF8临时路径
    @param [in] destination UTF8目标路径，根目录权限及包含关系由宿主验证
    @param [in] replace 是否允许覆盖既有目标，默认false
    @return 是否成功；失败时目标可能已发布但目录flush失败，宿主查验后恢复
    */
    static bool publishFileUtf8(const char* source, const char* destination, bool replace = false);

    /** 读取操作系统密码学随机源，不创建网络、线程或文件，不做弱随机回退
    @param [out] output size非零时不可空，仅借用至返回
    @param [in] size 单次0至8192字节；非法范围不访问输出
    @return 完整成功true；有效范围内失败擦除全部输出，熵尚未就绪/平台不支持返回false
    */
    static bool secureRandomBytes(uint8_t* output, uint32_t size);

    /** 为需要上下文及0成功返回值的密码学库提供同一系统强随机入口
    @param [in] context 忽略，不保存
    @param [out] output size非零时不可空
    @param [in] size 0至8192字节
    @return 0完整成功，-1失败；范围和清理语义同secureRandomBytes
    */
    static int32_t secureRandomBytesCallback(void* context, uint8_t* output, uint32_t size);
#endif // CSYSTEM_NATIVE_API

private:
#if CSYSTEM_HAS_NATIVE_API
    /** 判断存在路径是普通目录，不接受链接或重解析点
    @param [in] path 待检查目录
    @return 普通无链接目录返回true
    */
    static bool plainDirectory(const std::string& path);

    /** 判断存在路径是普通文件，不接受链接或重解析点
    @param [in] path 待检查文件
    @return 普通无链接文件返回true
    */
    static bool plainFile(const std::string& path);

    /** 以固定缓冲逐字节比较两个文件
    @param [in] first 第一文件
    @param [in] second 第二文件
    @return 两个文件完整读取且全部字节相同返回true
    */
    static bool equalFiles(const std::string& first, const std::string& second);

    /** 保留迁移文件时间，使依赖修改时间的内容版本保持稳定
    @param [in] source 已验证源文件
    @param [in] destination 已复制的目标文件
    @return 时间读取及写入成功返回true
    */
    static bool copyFileMetadata(const std::string& source, const std::string& destination);

    /** 复制到目标同目录临时文件，核对后无覆盖提交，再删源文件
    @param [in] source 普通源文件，调用方保证迁移期间没有并发写入
    @param [in] destination 目标路径
    @param [out] errorText 错误原因
    @return 目标字节完整、源移除成功返回true
    */
    static bool migrateFile(const std::string& source, const std::string& destination, std::string& errorText);

    /** 迁移已经核对为非重叠的目录树
    @param [in] source 普通源目录，调用方保证迁移期间没有并发写入
    @param [in] destination 普通目标目录
    @param [out] errorText 错误原因
    @return 所有子项完成且源空目录移除返回true
    */
    static bool migrateTree(const std::string& source, const std::string& destination, std::string& errorText);

#ifdef _WIN32

    /** 枚举本次请求的可见窗口，找到后停止
    @param [in] window 当前顶层窗口
    @param [in,out] context 同步调用期间有效的进程ID与结果对
    @return 继续枚举返回TRUE，匹配后返回FALSE
    */
    static BOOL CALLBACK findVisibleWindowCallback(HWND window, LPARAM context);

#endif

#if defined(_WIN32)
    /** 两种启动入口共用参数转义和进程所有权处理
    @param [in] executable UTF8绝对EXE路径
    @param [in] arguments 未转义参数
    @param [in] output 为空时不捕获，否则独占创建日志
    @param [out] pid 成功进程号
    @param [out] monitor 成功监视句柄
    @return 0或系统错误码
    */
    static uint32_t startOwnedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& output, uint32_t& pid, uint64_t& monitor);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 按单字符分隔并保留空字段，先清空输出数组
    @param [out] result 本次切分结果
    @param [in] splitString 待切分字节串
    @param [in] separateCharacter 单字节分隔符
    */
    static void split(std::vector<std::string>& result, const std::string& splitString, char separateCharacter);

    /** 按完整字符串分隔并保留空字段，空分隔符保留原输入
    @param [out] result 本次切分结果
    @param [in] splitString 待切分字节串
    @param [in] separateCharacter 分隔字符串，可空
    */
    static void split(std::vector<std::string>& result, const std::string& splitString, const std::string& separateCharacter);

    /** 向可选错误输出写入固定错误文本
    @param [out] errorText 可空，非空时替换内容
    @param [in] value 本次错误文本
    */
    static void setSystemError(std::string* errorText, const std::string& value);

    /** 将系统路径宽字符串转换为UTF8，保留内嵌NUL以供调用入口校验
    @param [in] text 借用至返回的宽字符串，允许为空
    @return 转换结果；空输入、转换或分配失败返回空串，Windows沿用系统flags=0的替代字符规则
    */
    static std::string widePathToUtf8(const std::wstring& text);

    /** 将UTF8路径转换为系统宽字符串，保留内嵌NUL以供调用入口校验
    @param [in] text 借用至返回的UTF8字节串，允许为空
    @return 转换结果；空输入、转换或分配失败返回空串，Windows沿用系统flags=0的替代字符规则
    */
    static std::wstring utf8PathToWide(const std::string& text);

#if CSYSTEM_HAS_DESKTOP_API
    /** 在控制台读取一个不回显的字符，POSIX读取后恢复原终端设置
    @return 字符值；标准输入到达末尾时返回EOF
    */
    static int getConsoleCharacter();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 保存文件操作错误并统一返回失败
    @param [out] error 可空，非空时写入系统错误码
    @param [in] value Win32错误码或errno
    @return false
    */
    static bool systemFileFailure(uint32_t* error, uint32_t value);

    /** 检查原生文件句柄的数值范围，避免POSIX宽整数被截断为另一个描述符
    @param [in] handle Windows句柄或POSIX文件描述符
    @return 数值可表示且不为无效句柄时返回true
    */
    static bool isSystemFileHandle(intptr_t handle);

    /** 检查句柄指向可寻址普通文件，拒绝目录及重解析点
    @param [in] handle 已打开且数值已校验的句柄
    @param [out] error 可空，失败写入系统错误码
    @return 是普通文件时返回true
    */
    static bool validateSystemFileHandle(intptr_t handle, uint32_t* error);

#ifdef _WIN32

    /** 枚举目标进程的第一个顶层窗口，匹配后停止，不限制窗口可见性
    @param [in] window 系统回调提供的窗口
    @param [in,out] context 同步借用的进程ID与窗口结果对
    @return 匹配时FALSE，否则TRUE继续枚举
    */
    static BOOL CALLBACK findWindowCallback(HWND window, LPARAM context);

    /** 为旧Windows申请本模块的计时精度租约，并在模块卸载时成对释放
    @param [in] once 系统初始化状态，不保存
    @param [in] parameter 未使用，不保存
    @param [out] context 未使用，不写入
    @return 初始化完成或系统能力不可用返回TRUE；分配异常返回FALSE允许重试
    */
    static BOOL CALLBACK initializeTimerResolution(PINIT_ONCE once, PVOID parameter, PVOID* context);

    /** 兼容旧Windows动态取得真实本机系统信息
    @param [out] information 可空，非空时写入系统信息；缺少新入口时使用GetSystemInfo
    */
    static void getNativeSystemInfo(LPSYSTEM_INFO information);
#endif

#ifdef _MSC_VER

    /** 枚举指定DirectShow设备类别
    @param [in] category 系统设备类别标识
    @param [out] enumeration 不可空，成功后调用方负责Release
    @return 成功S_OK；空输出E_POINTER；没有设备VFW_E_NOT_FOUND；其他情况返回COM错误
    */
    static HRESULT enumerateDevices(REFGUID category, IEnumMoniker** enumeration);

    /** 从DirectShow枚举器取得设备名称并追加到结果
    @param [in] enumeration 可空，借用至返回，不释放枚举器
    @param [in,out] names 设备名称列表，已有内容保留
    */
    static void displayDeviceInformation(IEnumMoniker* enumeration, std::vector<std::string>& names);

    /** 取得当前代码所在模块并增加系统模块引用
    @return 模块句柄，失败为空；调用方沿用既有进程级持有方式
    */
    static HMODULE getCurrentModule();
#endif

#if defined(__unix__) || defined(__APPLE__)

#if CSYSTEM_HAS_DESKTOP_API
    /** 读取当前CPU的周期计数，仅供既有频率测量使用
    @return 支持的指令计数；不支持的平台返回0，不作为跨进程时钟
    */
    static uint64_t getCycleCount();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 使用动态加载器信息取得当前模块目录
    @param [out] pathBuffer 可空，非空时必须至少有1024字节，失败写空串
    @return 原缓冲指针；传入空指针返回nullptr
    */
    static char* getModuleCurrentPath(char* pathBuffer);
#endif

#if defined(__linux__)

    /** 从同一次proc stat快照确认整个线程组退出
    @param [in] line proc stat完整内容，进程名允许空格及括号
    @return 仅Z或X状态且字段20明确为1时true；缺字段或还有其他线程时false
    */
    static bool linuxProcessStatExited(const std::string& line);
#endif

#ifdef CSYSTEM_SHARED_IDENTITY

    /** 取得本进程启动期配置的身份登记目录
    @return 进程内配置引用，仅允许setProcessDirectory在首次登记前串行修改
    */
    static std::string& processDirectory();

    /** 取得身份登记目录的封存标记，首次发布尝试后禁止更换路径
    @return 进程内原子标记引用，不承担目录字符串的并发修改同步
    */
    static std::atomic<bool>& processDirectorySealed();

    /** 一次性生成并发布本进程的随机实例身份，初始化失败可重试
    @return 本进程非零64位身份；随机源、目录或登记失败返回0
    */
    static uint64_t mobileSelfIdentity();

#ifdef _WIN32
    /** 由系统InitOnce初始化模拟移动后端的身份，避免VS2013的call_once异常重试兼容问题
    @param [in] once 系统初始化状态，不保存
    @param [out] parameter 不可空，指向进程级64位身份存储，仅完整发布后写入
    @param [out] context 未使用，不写入
    @return 完整发布返回TRUE；失败返回FALSE允许后续重试，不向系统回调传播异常
    */
    static BOOL CALLBACK initializeProcessIdentity(PINIT_ONCE once, PVOID parameter, PVOID* context);

#endif

    /** 通过同目录临时文件原子替换发布完整身份，不使用共享映射或跨进程文件锁
    @param [in] identity 非零随机64位身份
    @return 完整记录已发布时true；失败清理本次临时文件并保留原记录
    */
    static bool publishProcessIdentity(uint64_t identity);

    /** 读取参与进程的完整身份登记，不创建缺失记录
    @param [in] processId 已校验且尚未明确退出的正进程号
    @param [out] identity 成功写入非零身份，失败置0
    @return 取得完整有效记录时true；缺失、类型错误或读取失败为false
    */
    static bool readProcessIdentity(uint32_t processId, uint64_t& identity);

#ifndef _WIN32

    /** 打开宿主已有身份登记目录，拒绝末级链接及其他UID拥有的目录
    @param [in] directory 不含NUL的绝对路径
    @return 目录文件描述符，调用方负责close；非法或系统失败返回-1
    */
    static int openProcessDirectory(const std::string& directory);
#endif
#endif
#endif // CSYSTEM_NATIVE_API
};

#include "CSystem.inl"