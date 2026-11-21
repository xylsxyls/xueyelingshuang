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
#include <thread>
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

/** 对指定系统线程设置相对调度优先级的结果。
*/
enum CSystemThreadPriorityResult
{
    // 操作系统已应用请求。
    CSystemThreadPriorityApplied,
    // 传入线程不可用或等级超出范围。
    CSystemThreadPriorityInvalid,
    // 当前平台没有所请求的调度能力。
    CSystemThreadPriorityUnsupported,
    // 获得操作系统或标准库错误，详见 nativeError。
    CSystemThreadPriorityFailed
};

/** 系统工具集合，只依赖标准库和系统接口，封装系统信息、文件目录、进程、时间及平台专属能力
所有函数均为静态成员；移动端身份登记目录由宿主在启动期配置，其他生命周期由各接口明确约定
*/
class CSystemAPI CSystem
{
public:
    /** 设置 joinable 线程的调度等级，调用方负责与 join、detach 及析构互斥。
    @param [in] thread 已启动的线程
    @param [in] relative 相对普通等级 -2 至 2，Windows 支持全部等级；Linux/macOS/Android/iOS 支持普通和两种实时等级，实时调度可能需要权限；Wasm 返回 Unsupported
    @param [out] nativeError 系统失败时记录原生错误码，其余情况清零
    @return 应用成功、非法参数、平台不支持或系统失败
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm。Wasm 可调用，合法线程请求返回 Unsupported。
    */
    static CSystemThreadPriorityResult setThreadPriority(std::thread& thread, int32_t relative, int32_t& nativeError);

#if CSYSTEM_HAS_NATIVE_API
    /** 配置移动端身份登记专用目录，不借用其他库的配置
    Android使用应用内部专用目录，iOS使用App Group容器中的专用目录
    @param [in] directory 已存在且属于当前UID的绝对路径；首次登记前串行配置，各参与进程使用同一路径
    @return 配置成功true；空值、非法目录或首次登记尝试后更换路径为false；桌面原生后端只接受空路径
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool setProcessDirectory(const std::string& directory);

#if CSYSTEM_HAS_DESKTOP_API
    /** 取得桌面整机命名进程租约；内核锁在崩溃时自动释放
    @param [in] name ASCII角色名，所有安装目录和版本使用同一个名称
    @param [in] replace 已占用时请求旧进程正常退出并等待
    @param [in] timeoutMs 替换等待上限，最多30000毫秒
    @param [out] state 不透明租约；成功后由同一线程释放
    @return 1取得，0已有实例应复用，-1系统失败或替换超时
    @note 支持平台 Windows、Linux、macOS
    */
    static int32_t acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
        std::vector<uint64_t>& state);

    /** 按角色与隔离后缀取得锁，就绪记录始终按角色和进程号查询
    @param [in] name 用于查询就绪记录的角色名
    @param [in] replace 是否要求旧实例正常退出
    @param [in] timeoutMs 等待上限，最多30000毫秒
    @param [out] state 成功取得的独占租约
    @param [in] isolated 隔离后缀，最多 40 个 ASCII 字母、数字、点、下划线或连字符；空串使用角色名
    @return 1取得，0已有实例，-1失败
    @note 支持平台 Windows、Linux、macOS
    */
    static int32_t acquireProcessLease(const std::string& name, bool replace, uint32_t timeoutMs,
        std::vector<uint64_t>& state, const std::string& isolated);

    /** 发布或撤销本进程可接入状态，只有完整初始化后才置true
    @param [in] state 已取得租约
    @param [in] ready 当前是否可接入
    @return 系统操作是否成功
    @note 支持平台 Windows、Linux、macOS
    */
    static bool setProcessLeaseReady(const std::vector<uint64_t>& state, bool ready);

    /** 查询替换请求，由主循环转换为正常关闭
    @param [in] state 已取得租约
    @return 是否已收到停止请求；无效状态返回true
    @note 支持平台 Windows、Linux、macOS
    */
    static bool processLeaseStopRequested(const std::vector<uint64_t>& state);

    /** 撤销就绪并释放内核句柄；只能在所有工作已结束后调用
    @param [in,out] state 成功取得的租约，返回时清空
    @note 支持平台 Windows、Linux、macOS
    */
    static void releaseProcessLease(std::vector<uint64_t>& state);

    /** 验证指定PID已发布就绪；实际接入仍须完成IPC和TLS握手
    @param [in] name 角色名
    @param [in] pid 正进程号
    @return 当前已就绪；启动中、停止中及不存在均为false
    @note 支持平台 Windows、Linux、macOS
    */
    static bool isProcessLeaseReady(const std::string& name, uint32_t pid);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32)
    /** 设置当前用户的Windows登录启动项，仅Windows公开
    @param [in] name ASCII启动项名，只允许字母、数字、点、下划线和连字符，1至80字节
    @param [in] command UTF8完整命令行，Windows最多260个宽字符；空串只移除指定启动项
    @return 0成功或系统错误码；不创建服务或启动进程
    @note 支持平台 Windows
    */
    static uint32_t setUserLoginStartup(const std::string& name, const std::string& command);

    /** 以独立后台进程启动绝对路径，不经shell，仅Windows公开
    @param [in] executable UTF8绝对路径
    @param [in] arguments 原始UTF8参数，各自引用转义，最多64项
    @param [out] pid 成功子进程号，失败0
    @param [out] monitor 成功取得的独占监视句柄，须closeProcessMonitor；不结束子进程
    @return 0或系统错误码，不继承宿主句柄
    @note 支持平台 Windows
    */
    static uint32_t startDetachedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        uint32_t& pid, uint64_t& monitor);

    /** 后台启动进程并独占创建输出文件，不经shell，仅Windows公开
    @param [in] executable UTF8绝对EXE路径
    @param [in] arguments 逐项原始参数，最多64项
    @param [in] output UTF8绝对日志路径，必须不存在；只继承该文件和NUL输入
    @param [out] pid 成功进程号，失败0
    @param [out] monitor 独占监视句柄，由closeProcessMonitor释放
    @return 0或系统错误码；不会覆盖已有日志，不结束其他进程
    @note 支持平台 Windows
    */
    static uint32_t startCapturedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& output, uint32_t& pid, uint64_t& monitor);

    /** 采样有效监视句柄所属进程资源，仅Windows公开
    @param [in] monitor 0为当前进程，其他为本库取得且未释放的句柄，需具备查询和读取权限
    @param [out] privateBytes 私有提交字节
    @param [out] workingSet 当前工作集字节
    @param [out] cpuTime100ns 内核和用户CPU时间之和，单位100纳秒
    @param [out] handles 进程句柄数
    @param [out] threads 进程线程数
    @return 完整采样成功true；失败各输出清0，不把无权限当已退出
    @note 支持平台 Windows
    */
    static bool queryProcessResources(uint64_t monitor, uint64_t& privateBytes, uint64_t& workingSet,
        uint64_t& cpuTime100ns, uint32_t& handles, uint32_t& threads);

    /** Windows打开进程退出监视句柄，持有期间不会混淆复用PID
    @param [in] pid 目标进程号
    @return 独占句柄，失败为0；权限不足不当成已退出
    @note 支持平台 Windows
    */
    static uint64_t openProcessMonitor(uint32_t pid);

    /** 非阻塞查询已持有进程的退出事实
    @param [in] monitor 有效句柄
    @param [out] exitCode 已退出时为实际退出码，其他情况0
    @return 1已退出，0仍运行，-1错误
    @note 支持平台 Windows
    */
    static int32_t pollProcessMonitor(uint64_t monitor, int32_t& exitCode);

    /** 关闭监视句柄，不关闭目标进程
    @param [in,out] monitor 本模块创建的独占句柄，释放后0
    @note 支持平台 Windows
    */
    static void closeProcessMonitor(uint64_t& monitor);

    /** Windows无控制台程序将CRT输出绑定到调用方明确重定向的管道/文件
    @return 绑定成功；没有重定向时为true，不分配或附加控制台
    @note 支持平台 Windows
    */
    static bool bindInheritedOutput();
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif // CSYSTEM_NATIVE_API

#ifdef _WIN32

    /** 获取系统任务栏矩形大小
    @return 返回任务栏矩形，获取失败时字段通常为0
    @note 支持平台 Windows
    */
    static RECT GetTaskbarRect();

    /** 获取屏幕分辨率
    @return 返回屏幕矩形，right/bottom表示宽高边界
    @note 支持平台 Windows
    */
    static RECT GetWindowResolution();

    /** 返回屏幕右下角坐标，不包含任务栏
    @return 返回可见工作区右下角坐标
    @note 支持平台 Windows
    */
    static POINT taskbarRightBottomPoint();

    /** 返回屏幕中央点
    @return 返回主屏幕中心坐标
    @note 支持平台 Windows
    */
    static POINT screenCenterPoint();

    /** 将字符串写入到剪贴板
    @param [in] hWnd 有效的所有者窗口句柄，传空时直接返回
    @param [in] str 要写入剪贴板的Ansi字符串
    @note 支持平台 Windows
    */
    static void setClipboardData(HWND hWnd, const std::string& str);

    /** 获取剪贴板内容
    @param [in] hWnd 打开剪贴板时使用的窗口句柄
    @return 返回剪贴板中的Ansi文本，没有文本时返回空字符串
    @note 支持平台 Windows
    */
    static std::string GetClipboardData(HWND hWnd);

    /** 获取控制台窗口句柄
    @return 返回当前进程控制台窗口句柄，获取失败返回NULL
    @note 支持平台 Windows
    */
    static HWND GetConsoleHwnd();

    /** 根据进程ID获取窗口句柄
    @param [in] dwProcessId 进程ID
    @return 返回匹配进程的窗口句柄，找不到返回NULL
    @note 支持平台 Windows
    */
    static HWND GetHwndByProcessId(uint32_t dwProcessId);

    /** 查找指定进程的可见顶层窗口，不改变原GetHwndByProcessId的筛选语义
    @param [in] processId 进程ID，0不匹配
    @return 首个匹配的可见窗口，未找到返回nullptr；仅Windows提供
    @note 支持平台 Windows
    */
    static HWND findVisibleWindowByProcessId(uint32_t processId);

    /** 返回OCX注册路径
    @param [in] classid OCX的ClassId
    @return 返回OCX所在目录，带路径分隔符，失败返回空字符串
    @note 支持平台 Windows
    */
    static std::string GetRegOcxPath(const std::string& classid);

    /** 禁用WOW64文件系统重定向
    @return 返回恢复重定向时需要传回的旧值
    @note 支持平台 Windows
    */
    static void* ForbidRedir();

    /** 恢复WOW64文件系统重定向
    @param [in] oldValue ForbidRedir返回的旧值
    @note 支持平台 Windows
    */
    static void RecoveryRedir(void* oldValue);

    /** 读取主显示器全屏窗口的客户区高度指标
    @return 返回 GetSystemMetrics(SM_CYFULLSCREEN) 的像素值
    @note 支持平台 Windows
    */
    static int GetVisibleHeight();

    /** 使用系统默认方式打开网页
    @param [in] webPage 网页URL
    @note 支持平台 Windows
    */
    static void OpenWebPage(const std::string& webPage);

    /** 判断当前鼠标左键是否按下
    @return 返回左键是否处于按下状态
    @note 支持平台 Windows
    */
    static bool isMouseLeftDown();

    /** 判断当前鼠标右键是否按下
    @return 返回右键是否处于按下状态
    @note 支持平台 Windows
    */
    static bool isMouseRightDown();

    /** 判断当前鼠标中键是否按下
    @return 返回中键是否处于按下状态
    @note 支持平台 Windows
    */
    static bool isMouseMidDown();

    /** 复制文件或目录，目标目录不存在时会自动创建
    @param [in] from 源路径
    @param [in] dest 目标路径
    @return 返回是否复制成功
    @note 支持平台 Windows
    */
    static bool ShellCopy(const char* from, const char* dest);

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取第一个和名字匹配的进程ID
    @param [in] processNameW 进程名宽字符串
    @return 返回进程ID，找不到返回0
    @note 支持平台 Windows
    */
    static uint32_t processFirstPid(const std::wstring& processNameW);

    /** 获取指定进程名的全部进程ID
    @param [in] processNameW 进程名宽字符串
    @return 返回进程ID列表
    @note 支持平台 Windows
    */
    static std::vector<uint32_t> processPid(const std::wstring& processNameW);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 根据进程ID获取进程名
    @param [in] pid 进程ID
    @return 返回进程名宽字符串，失败返回空字符串
    @note 支持平台 Windows
    */
    static std::wstring processNameW(uint32_t pid);

    /** 获取指定进程挂载的动态库列表
    @param [in] pid 进程ID
    @return 返回模块文件名列表（含可执行模块），各项不含目录；枚举失败可返回空列表
    @note 支持平台 Windows
    */
    static std::vector<std::string> processMountDll(uint32_t pid);

    /** 获取所有摄像头名称
    @return 返回摄像头名称列表，内部会调用CoUninitialize
    @note 支持平台 Windows
    */
    static std::vector<std::string> allCameraName();

    /** 获取所有设备信息
    @return 返回设备类型到设备信息列表的映射
    @note 支持平台 Windows
    */
    static std::map<std::string, std::vector<std::string>> allDeviceInfo();

#endif

#if CSYSTEM_HAS_NATIVE_API
#if CSYSTEM_HAS_DESKTOP_API
    /** 查询操作系统公开的 CPU 频率
    @return 单位 GHz。Windows 读取 CPU 0 的注册表频率，Linux 读取 CPU 0 的最大频率，macOS 查询 hw.cpufrequency；系统未公开时返回 0
    @note 支持平台 Windows、Linux、macOS。
    */
    static double GetCPUSpeedGHz();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 跨平台线程休眠
    @param [in] milliseconds 休眠时间，单位毫秒
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void Sleep(long long milliseconds);

    /** 网络轮询的短时阻塞等待，Windows 普通计时器回退首次按需申请 1 毫秒精度
    @param [in] milliseconds 等待毫秒；Windows支持时使用高精度等待计时器，其他情况回退Sleep
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void sleepPrecise(uint32_t milliseconds);

    /** 生成UUID字符串
    @param [in] flag 1表示带横线，0表示不带横线
    @return 返回小写UUID字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string uuid(int flag = 1);

    /** 复制文件，覆盖同一文件保留原内容。路径内嵌 NUL 时忽略本次操作
    @param [in] dstFile 目标文件路径
    @param [in] srcFile 源文件路径
    @param [in] over 目标存在时是否覆盖
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void CopyFileOver(const std::string& dstFile, const std::string& srcFile, bool over);

    /** 查询 Windows 系统架构位数或 POSIX 当前进程指针位数
    @return 返回32或64，无法判断时返回0
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static int GetSystemBits();

    /** 获取当前系统用户名
    @return Windows 返回调用线程的用户名，POSIX 先按进程真实用户 ID 查询，无法取得时返回空串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetSysUserName();

    /** 获取环境变量值
    @param [in] name 环境变量名
    @return 返回环境变量值，不存在返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetEnvironment(const char* name);

    /** 创建目录
    @param [in] dir 目录路径，带不带路径分隔符均可
    @return 返回是否创建成功，目录已存在或多级父目录不存在时返回false
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool CreateDir(const std::string& dir);

    /** 删除空目录
    @param [in] dir 目录路径，带不带路径分隔符均可
    @return 返回是否删除成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool DestroyDir(const std::string& dir);

    /** 检测目录或文件是否存在
    @param [in] dir 目录或文件路径，目录带不带路径分隔符均可
    @return 返回路径是否存在
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool DirOrFileExist(const std::string& dir);

    /** 判断普通文件是否存在，支持Windows宽字符路径
    @param [in] path 文件路径
    @return 文件存在且不是目录时返回true，否则返回false
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool fileExists(const std::wstring& path);

#if CSYSTEM_HAS_DESKTOP_API
    /** 打开系统文件夹
    @param [in] folder 文件夹路径
    @note 支持平台 Windows、Linux、macOS
    */
    static void OpenFolder(const std::string& folder);

    /** 打开系统文件夹并选中文件
    @param [in] file 文件路径
    @note 支持平台 Windows、Linux、macOS
    */
    static void OpenFolderAndSelectFile(const std::string& file);

    /** 使用系统或指定外部程序打开文件
    @param [in] file 文件路径，Linux下可包含外部程序名，例如gedit
    @note 支持平台 Windows、Linux、macOS
    */
    static void OpenFile(const std::string& file);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 退出fork创建的子进程，不执行父进程继承来的全局析构和atexit逻辑
    @param [in] exitCode 子进程退出码
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void childProcessExit(int32_t exitCode);
#endif // CSYSTEM_NATIVE_API

    /** 创建动态二维数组
    @param [in] row 行数
    @param [in] column 列数
    @return 各行分别分配，尺寸非正或分配返回空时返回空指针；元素构造异常在已构造元素释放后向上传播
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    template <typename TypeClass>
    static TypeClass** CreateDyadicArray(int32_t row, int32_t column);

    /** 销毁动态二维数组
    @param [in] classPtr CreateDyadicArray返回的二维数组指针
    @param [in] row 行数
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    template <typename TypeClass>
    static void DestroyDyadicArray(TypeClass** classPtr, int32_t row);

    /** 将RECT风格对象中小于0的点修正为0
    @param [in] rect 原矩形对象，要求有left/top/right/bottom成员
    @return 返回修正后的矩形对象
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    template <typename TRECT>
    static TRECT rectValid(const TRECT& rect);

    /** 将QRect风格对象中小于0的点修正为0
    @param [in] rect 原矩形对象，要求有left/top/right/bottom访问器和set函数
    @return 返回修正后的矩形对象
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    template <typename TQRect>
    static TQRect qrectValid(const TQRect& rect);

#if CSYSTEM_HAS_NATIVE_API
    /** 输出字符串map到控制台或文件
    @param [in] stringMap 要输出的map
    @param [in] path 输出文件路径，空字符串表示输出到控制台
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void OutputMap(const std::map<std::string, std::string>& stringMap, const std::string& path = "");

    /** 输出字符串vector到控制台或文件
    @param [in] stringVector 要输出的vector
    @param [in] path 输出文件路径，空字符串表示输出到控制台
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void OutputVector(const std::vector<std::string>& stringVector, const std::string& path = "");

#if CSYSTEM_HAS_DESKTOP_API
    /** 从本模块的标准输入丢弃一行，遇到换行或 EOF 返回；与本模块 inputString 共用输入
    @note 支持平台 Windows、Linux、macOS
    */
    static void ClearScanf();

    /** 从调用方指定的字符流丢弃一行，适用于调用方和 DLL 分别静态链接 CRT
    @param [in] readCharacter 同步借用，通常传调用方的 getchar；每次返回一个无符号字节或 EOF，空指针直接返回
    @note 支持平台 Windows、Linux、macOS
    */
    static void ClearScanf(int (*readCharacter)());
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取执行参数列表
    @param [in] argc main函数参数数量，Windows下可传默认值
    @param [in] argv main函数参数数组，Windows下可传默认值
    @return 包含程序名的参数列表；Windows 取 CRT 当前进程参数，其余原生平台取传入数组，无效数组返回空列表
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::vector<std::string> exeParam(int argc = 0, char** argv = nullptr);

#if CSYSTEM_HAS_DESKTOP_API
    /** 从控制台读取密码
    @return 返回用户输入的密码文本
    @note 支持平台 Windows、Linux、macOS
    */
    static std::string PasswordScanf();
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 执行系统命令并获取输出
    @param [in] command 命令文本
    @param [out] result 命令输出字节，可含 NUL
    @param [in] isShowCmd Windows 为 false 时直接隐藏启动并合并标准错误，为 true 时使用 shell。POSIX 总是使用 shell
    @return 返回子进程退出码，内部错误返回 -1。POSIX 信号终止时返回 128 加信号编号
    @note 支持平台 Windows、Linux、macOS、Android。Android受应用沙箱限制
    */
    static int32_t SystemCommand(const std::string& command, std::string& result, bool isShowCmd = false);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取当前线程ID
    @return 返回当前系统线程ID
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static uint32_t SystemThreadId();

    /** 获取CPU硬件线程数
    @return 返回系统报告的硬件线程数，原生 POSIX 实现无法提供时可为 0
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static int32_t GetCPUCoreCount();

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
    /** 获取Windows或Linux桌面的版本编号
    @return Windows返回版本编码，Linux返回类似1604、1804的发行版版本号
    @note 支持平台 Windows、Linux
    */
    static int32_t GetSystemVersionNum();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 获取当前进程ID
    @return 返回当前进程ID
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static uint32_t currentProcessPid();

    /** 仅在操作系统明确证明进程已退出/不存在时返回true，不把权限不足当成死亡
    @param [in] processId 正进程号；0或不可表示的值返回false
    @return 明确退出为true，存活或无法判定为false；PID不构成通信身份认证
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool processDefinitelyExited(uint32_t processId);

    /** 查询进程实例身份。桌面使用系统创建时间；移动端使用同一共享目录中的参与者随机登记
    移动端首次调用前须配置CSystem::setProcessDirectory指定的目录；只认识已初始化本库的参与进程。
    @param [in] processId 正PID
    @param [out] identity 完整实例标识；失败置0，不代表目标已死亡
    @return 身份已知为true；权限不足、未登记或底层失败为false
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool processIdentity(uint32_t processId, uint64_t& identity);
#endif // CSYSTEM_NATIVE_API

    /** 读取进程命令行参数为UTF8，不包含程序名；Windows不依赖ANSI argv
    @param [in] argc 标准main参数个数1至4096；Windows GUI入口可用0并传空argv
    @param [in] argv 标准main参数；Windows使用系统宽字符命令行，GUI入口允许与argc=0成对为空
    @param [out] arguments 成功替换列表，参数错误或编码失败保持原值
    @return 是否成功；结果从程序名后的第一个参数开始，标准库分配异常由调用边界处理
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static bool commandArguments(int32_t argc, char** argv, std::vector<std::string>& arguments);

#if CSYSTEM_HAS_NATIVE_API
#if !defined(__APPLE__) || !TARGET_OS_IPHONE
    /** 获取第一个和名字匹配的进程ID
    @param [in] processName 进程名
    @return 返回进程ID，Linux/Android 返回可见匹配进程中最大的 PID，找不到返回0
    @note 支持平台 Windows、Linux、macOS、Android
    */
    static uint32_t processFirstPid(const std::string& processName);

    /** 获取指定进程名的全部进程ID
    @param [in] processName 进程名
    @return 返回进程ID列表，Linux/Android 按可见 PID 降序返回
    @note 支持平台 Windows、Linux、macOS、Android。Android进程可见范围受系统权限限制
    */
    static std::vector<uint32_t> processPid(const std::string& processName);

    /** 根据进程ID获取进程全名
    @param [in] pid 进程ID
    @return 返回进程名，失败返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android
    */
    static std::string processName(uint32_t pid);
#endif // CSYSTEM_PLATFORM_CAPABILITY

#if defined(_WIN32) || defined(__linux__)
    /** 获取当前进程占用内存
    @return 返回内存字节数，Linux下精度通常到KB
    @note 支持平台 Windows、Linux、Android
    */
    static uint64_t currentMemory();
#endif // CSYSTEM_PLATFORM_CAPABILITY
#endif // CSYSTEM_NATIVE_API

    /** 获取系统运行毫秒计数
    @return 返回毫秒计数，可能按系统API语义回绕
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static uint32_t GetTickCount();

    /** 获取高精度时间点
    @return 返回当前高精度时钟时间点
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static std::chrono::high_resolution_clock::time_point GetHighTickCount();

    /** 获取从开始时间到现在的执行耗时，单位毫秒
    @param [in] beginTime 开始时间点
    @return 返回耗时毫秒数，内部会从int64_t强转为int32_t
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int32_t GetHighTickCountMilliRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);

    /** 获取从开始时间到现在的执行耗时，单位微秒
    @param [in] beginTime 开始时间点
    @return 返回耗时微秒数，内部会从int64_t强转为int32_t
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int32_t GetHighTickCountMicroRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);

    /** 获取从开始时间到现在的执行耗时，单位纳秒
    @param [in] beginTime 开始时间点
    @return 返回耗时纳秒数
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int64_t GetHighTickCountNanoRunTime(const std::chrono::high_resolution_clock::time_point& beginTime);

    /** 获取两个时间点之间的耗时，单位毫秒
    @param [in] endTime 结束时间点
    @param [in] beginTime 开始时间点
    @return 返回耗时毫秒数，内部会从int64_t强转为int32_t
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int32_t GetMilliRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);

    /** 获取两个时间点之间的耗时，单位微秒
    @param [in] endTime 结束时间点
    @param [in] beginTime 开始时间点
    @return 返回耗时微秒数，内部会从int64_t强转为int32_t
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int32_t GetMicroRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);

    /** 获取两个时间点之间的耗时，单位纳秒
    @param [in] endTime 结束时间点
    @param [in] beginTime 开始时间点
    @return 返回耗时纳秒数
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static int64_t GetNanoRunTime(const std::chrono::high_resolution_clock::time_point& endTime, const std::chrono::high_resolution_clock::time_point& beginTime);

#if CSYSTEM_HAS_NATIVE_API
    /** 获取电脑名
    @return 返回当前主机名，失败返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string getComputerName();

    /** 获取当前动态库所在路径
    @return 返回动态库目录，通常带路径分隔符
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetCurrentDllPath();

    /** 获取当前进程所在目录
    @return 返回exe所在目录，带路径分隔符
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetCurrentExePath();

    /** 取得当前可执行文件完整UTF8路径，Windows 通过宽字符系统 API 查询
    @return 完整路径，失败空；不依赖当前工作目录
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string currentExecutablePathUtf8();

    /** 获取当前进程名，不带后缀名
    @return 返回进程名
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetCurrentExeName();

    /** 获取当前进程完整文件名
    @return 返回进程文件名，带后缀名
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetCurrentExeFullName();

    /** 获取本库约定的用户文件目录。Windows 使用当前用户的漫游 AppData，POSIX 使用 TMPDIR 或 /tmp
    @return 带末尾分隔符的目录路径，Windows 查询失败时返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string GetSystemTempPath();
#endif // CSYSTEM_NATIVE_API

    /** 获取路径中的文件信息
    @param [in] path 文件或目录路径
    @param [in] flag 1文件全名，2后缀名，3文件名，4前置路径并带路径分隔符
    @return 返回指定部分字符串，无法解析时返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static std::string GetName(const std::string& path, int32_t flag);

    /** 拼接目录和相对文件名，只处理末尾分隔符，不访问文件系统或解析..路径
    @param [in] directory 目录，可空；支持正斜杠和反斜杠，已有末尾分隔符不重复添加
    @param [in] name 相对文件名；调用方负责拒绝绝对路径和不允许的子目录
    @return 拼接路径；任一参数为空时返回另一参数
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static std::string joinPath(const std::string& directory, const std::string& name);

#if CSYSTEM_HAS_NATIVE_API
    /** 规范绝对路径，可按要求解析实际文件和目录链接
    @param [in] path 非空路径，不允许零字符
    @param [in] requireExisting 为true时解析实际存在目标，为false时规范词法绝对路径
    @return 规范路径；失败返回空，Windows比较用路径统一ASCII大小写
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string canonicalPath(const std::string& path, bool requireExisting = true);

    /** 明确UTF8输入/输出的绝对路径；Windows 通过宽字符系统 API 查询
    @param [in] path UTF8非空路径，不允许零字符
    @param [in] requireExisting true解析实际目标，false规范绝对词法路径
    @return UTF8绝对路径，失败空；Windows统一ASCII大小写
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string canonicalPathUtf8(const std::string& path, bool requireExisting = true);

    /** 按UTF8路径创建普通目录及父目录，拒绝最终链接/重解析点
    @param [in] path 非空UTF8路径
    @return 完整普通目录已存在或成功创建
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool createDirectoriesUtf8(const std::string& path);

    /** 判断路径是否严格位于目录内，存在目标会解析链接
    @param [in] path 待核对路径
    @param [in] directory 允许的根目录
    @param [in] requireExisting 是否要求目标实际存在
    @return 存在有效规范路径且严格包含时返回true，目录自身不算子项
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool isPathWithin(const std::string& path, const std::string& directory, bool requireExisting = true);

    /** 将静止目录迁到不重叠目录，逐文件核对内容后才删除源
    @param [in] source 源目录；不存在视为已完成，拒绝符号链接和重解析点
    @param [in] destination 目标目录；同名文件不同内容时不覆盖
    @param [out] errorText 失败原因；失败保留尚未验证的源数据，可幂等重试
    @return 全部文件完成且源空目录移除时返回true；同步IO不应在交互线程运行
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool migrateDirectory(const std::string& source, const std::string& destination, std::string& errorText);

    /** 创建普通目录及缺失父目录，已有普通目录视为成功
    @param [in] path 待创建的非空目录路径，拒绝目标重解析点或符号链接
    @return 完整目录存在且为普通目录返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool createDirectories(const std::string& path);
#endif // CSYSTEM_NATIVE_API

    /** 保证文件扩展名，匹配已有后缀时规范其大小写，否则追加后缀
    @param [in] path 原路径，空路径保持空
    @param [in] extension 含点的目标ASCII扩展名，例如.pdf；空扩展名保持原路径
    @return 新路径，不访问文件系统；ASCII大小写不敏感，保留路径其他字节
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static std::string ensureFileExtension(const std::string& path, const std::string& extension);

#if CSYSTEM_HAS_NATIVE_API
    /** 删除文件或空目录
    @param [in] path 文件或目录路径，Windows 下只删除文件，POSIX 原生端也可删除空目录
    @return 返回是否删除成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool deleteFile(const char* path);

    /** 打开二进制输出文件，支持Windows宽字符路径
    @param [in] path 输出文件路径
    @param [out] errorText 打开失败原因，可为空
    @return 打开成功返回FILE指针，失败返回nullptr
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static FILE* openBinaryOutputFile(const std::wstring& path, std::string* errorText = nullptr);

    /** 排他创建仅当前用户、系统和管理员可访问的私密文件，已存在时不覆盖
    @param [in] path 完整临时路径；宿主负责在可信目录内随机命名和最终替换
    @param [out] errorText 固定失败文案，不包含私密文件内容，可空
    @return 由CSystem关闭的FILE；Windows保护DACL，Unix权限0600且不跟随符号链接
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static FILE* openPrivateBinaryOutputFile(const std::wstring& path, std::string* errorText = nullptr);

    /** 将已写数据刷新到操作系统持久介质，之后仍须检查close和最终原子替换
    @param [in] file 由CSystem创建且尚未关闭的输出文件
    @return fflush及系统提交成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool flushBinaryOutputFile(FILE* file);

    /** 将已完成的同目录临时文件排他安装，目标存在时保持原目标不变
    @param [in] sourcePath 已提交并关闭的临时文件
    @param [in] destinationPath 正式路径
    @return 成功安装；失败时源可能仍存在，宿主只能清理自己创建的临时文件
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool installFileIfAbsent(const std::wstring& sourcePath, const std::wstring& destinationPath);

    /** 按UTF8检查普通文件，拒绝目录、符号链接及Windows重解析点
    @param [in] path 非空UTF8路径，不允许NUL
    @return 普通文件存在
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool plainFileUtf8(const std::string& path);

    /** 读取普通文件的字节数与修改标记，拒绝符号链接和重解析点
    @param [in] path 完整UTF8路径
    @param [out] bytes 字节数，须与 stamp 指向不同对象，失败时清零所有已提供的输出
    @param [out] stamp 修改标记，两个输出均不可空，仅供比较，不作为跨平台 UTC 时间
    @return 成功返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool fileStampUtf8(const std::string& path, uint64_t* bytes, uint64_t* stamp);

    /** 写入由 openBinaryOutputFile 创建的文件，避免跨 DLL 使用不同 CRT 的 FILE
    @param [in] file CSystem 创建的输出文件
    @param [in] data 非空的待写字节缓冲，借用至返回
    @param [in] size 字节数量
    @return 完整写入返回 true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool writeBinaryOutputFile(FILE* file, const void* data, size_t size);

    /** 关闭 CSystem 创建的输出文件并检查缓冲写入结果
    @param [in] file 输出文件，关闭后不再有效
    @return 刷新及关闭成功返回 true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool closeBinaryOutputFile(FILE* file);

    /** 删除文件或空目录，支持Windows宽字符路径
    @param [in] path 文件或目录路径，Windows 下只删除文件，POSIX 原生端也可删除空目录
    @return 返回是否删除成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool deleteFile(const std::wstring& path);

#if CSYSTEM_HAS_DESKTOP_API
    /** 控制台带提示读取一行字符串
    @param [in] tip 输入提示文本
    @return 返回用户输入内容
    @note 支持平台 Windows、Linux、macOS
    */
    static std::string inputString(const std::string& tip);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 关闭进程
    @param [in] pid 进程ID
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void killProcess(int32_t pid);

    /** 重命名文件或目录
    @param [in] oldPath 原文件或目录路径
    @param [in] newPath 新文件或目录路径，已存在的新文件或空目录会先删除
    @return 返回是否重命名成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool rename(const std::string& oldPath, const std::string& newPath);

    /** 将同文件系统临时文件替换目标文件，不预先删除目标
    @param [in] sourcePath 已写完并关闭的临时文件，失败时保留
    @param [in] destinationPath 目标宽字符路径，允许已存在
    @return 替换成功返回 true；失败保留原目标
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool replaceFile(const std::wstring& sourcePath, const std::wstring& destinationPath);
#endif // CSYSTEM_NATIVE_API

    /** 将time_t转换为时间字符串
    @param [in] timet 时间值
    @param [in] isLocal 是否按本地时间转换，false表示格林威治时间
    @return 返回格式化后的时间字符串，转换失败或结果超出 19 字节时返回空串
    @note 支持平台 Windows、Linux、macOS、Android、iOS、Wasm
    */
    static std::string timetToStr(time_t timet, bool isLocal = true);

#if CSYSTEM_HAS_NATIVE_API
    /** 在 XUEYELINGSHUANG 指定的 common 目录递归选择带数值版本的程序
    @param [in] name 文件名中位于版本号之前的前缀
    @return 匹配到的最高正数版本路径；没有匹配时返回空串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string commonFile(const std::string& name);

    /** 读取整个文件内容
    @param [in] path 文件路径
    @return 返回文件内容，失败返回空字符串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string readFile(const std::string& path);

    /** 按预算读取二进制文件，逐块限制内存，不依赖预先查询的文件大小
    @param [in] path Unicode路径，不能为空或含零字符
    @param [in] maximumBytes 允许读取的最大字节数，超出即失败
    @param [out] data 成功替换结果，打开、读取、超限或关闭失败保留原值
    @return 完整读取且关闭成功返回true；分配异常传播但文件会关闭；同步IO须在合适的执行线程调用
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool readBinaryFile(const std::wstring& path, size_t maximumBytes, std::vector<uint8_t>& data);

    /** 保存文件内容
    @param [in] content 要写入的内容
    @param [in] path 文件路径
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void saveFile(const std::string& content, const std::string& path);

    /** 查找文件，内部没有禁止路径重定向
    @param [in] strPath 根目录，可省略末尾分隔符，空串表示当前可执行文件所在目录
    @param [in] flag 1表示查找文件，2表示查找文件后缀名，3表示查找所有文件，fileStr不起作用
    @param [in] fileStr 传文件名带后缀名或后缀名不带点，多个后缀名点分隔，模式 2 保留空字段以匹配无后缀文件。后缀取实际目录项最后一个点之后的字节
    @param [in] EveryFilePath 在每添加一个文件路径之前调用此函数，返回true表示中断搜索，继续搜索返回false
    @param [in] unVisitPath 非空时清空并收集未能打开的目录；传空跳过记录
    @return 按遍历次序返回匹配路径；传相对根目录时结果也可以是相对路径
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::vector<std::string> findFilePath(const std::string& strPath,
        int32_t flag = 3,
        const std::string& fileStr = "",
        const std::function<bool (const std::string&)>& EveryFilePath = nullptr,
        std::vector<std::string>* unVisitPath = nullptr);

    /** 按 UTF8 路径打开可寻址普通文件，拒绝末级链接、重解析点及 FIFO
    @param [in] path 非空UTF8路径，最长4096
    @param [in] mode 1既有只读，2独占新建读写不覆盖，3既有读写不截断
    @param [out] error 可空，Win32错误或errno，成功0
    @return 独占句柄，失败-1；调用者负责closeFileHandle，不在网络/GUI执行者做慢I/O
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static intptr_t openFileHandle(const char* path, uint32_t mode, uint32_t* error = nullptr);

    /** 独占创建可由其他执行者只读观察的输出文件，不覆盖既有文件
    @param [in] path UTF8绝对路径
    @param [out] error 可空系统错误
    @return 本模块文件句柄或-1；使用 write/flush/closeFileHandle；Windows 拒绝其他写者，POSIX 的 O_EXCL 仅保证首次创建
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static intptr_t createObservedOutputFile(const char* path, uint32_t* error = nullptr);

    /** 复制普通文件句柄，原件仍归宿主；Windows副本共享文件位置，须同一执行者使用
    @param [in] native Windows HANDLE的intptr_t，其余平台fd
    @param [out] error 可空系统错误
    @return 新独占句柄，失败-1
    @note 支持平台 Windows、Linux、macOS、Android、iOS
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
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool readFileHandle(intptr_t handle, uint64_t offset, uint8_t* output, uint32_t capacity, uint32_t* received, uint32_t* error = nullptr);

    /** 完整位置写；失败可能已写前缀，写完不等于持久化
    @param [in] handle 可写普通文件句柄
    @param [in] offset 位置，offset+size不超过INT64_MAX
    @param [in] input size非零时不可空
    @param [in] size 最大16MiB
    @param [out] error 可空系统错误
    @return 是否完整写入
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool writeFileHandle(intptr_t handle, uint64_t offset, const uint8_t* input, uint32_t size, uint32_t* error = nullptr);

    /** 将已写文件数据持久化，失败不得提前确认/发布
    @param [in] handle 可写句柄
    @param [out] error 可空系统错误
    @return 是否持久化成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool flushFileHandle(intptr_t handle, uint32_t* error = nullptr);

    /** 查询实际文件长度
    @param [in] handle 句柄
    @param [out] bytes 不可空，失败0
    @param [out] error 可空系统错误
    @return 是否成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool fileHandleSize(intptr_t handle, uint64_t* bytes, uint32_t* error = nullptr);

    /** 设置文件长度，截断未确认尾部不代表持久化
    @param [in] handle 可写句柄
    @param [in] bytes 不超过INT64_MAX
    @param [out] error 可空系统错误
    @return 是否成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool resizeFileHandle(intptr_t handle, uint64_t bytes, uint32_t* error = nullptr);

    /** 关闭并将调用者句柄置-1；-1重复关闭成功，不隐式flush
    @param [in,out] handle 不可空，关闭后不得再用旧值
    @param [out] error 可空系统错误
    @return 是否关闭成功
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool closeFileHandle(intptr_t* handle, uint32_t* error = nullptr);

    /** 同文件系统发布已经持久化且关闭的普通临时文件
    @param [in] source UTF8临时路径
    @param [in] destination UTF8目标路径，根目录权限及包含关系由宿主验证
    @param [in] replace 是否允许覆盖既有目标，默认false
    @return 是否成功；失败时目标可能已发布但目录flush失败，宿主查验后恢复
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool publishFileUtf8(const char* source, const char* destination, bool replace = false);

    /** 读取操作系统密码学随机源，不创建网络、线程或文件，不做弱随机回退
    @param [out] output size非零时不可空，仅借用至返回
    @param [in] size 单次0至8192字节；非法范围不访问输出
    @return 完整成功true；有效范围内失败擦除全部输出，熵尚未就绪/平台不支持返回false
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool secureRandomBytes(uint8_t* output, uint32_t size);

    /** 为需要上下文及0成功返回值的密码学库提供同一系统强随机入口
    @param [in] context 忽略，不保存
    @param [out] output size非零时不可空
    @param [in] size 0至8192字节
    @return 0完整成功，-1失败；范围和清理语义同secureRandomBytes
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static int32_t secureRandomBytesCallback(void* context, uint8_t* output, uint32_t size);
#endif // CSYSTEM_NATIVE_API

private:
#if CSYSTEM_HAS_DESKTOP_API
    /** 取得角色名对应的锁、结束通知和就绪记录。
    @param [in] name 就绪查询使用的公共角色名
    @param [in] slot 已校验的锁名称
    @param [in] replace 是否要求旧实例退出
    @param [in] timeoutMs 等待上限，毫秒
    @param [out] state 成功时接收本线程所属的租约
    @return 1取得，0已有实例，-1失败
    @note 支持平台 Windows、Linux、macOS
    */
    static int32_t acquireNamedProcessLease(const std::string& name, const std::string& slot, bool replace,
        uint32_t timeoutMs, std::vector<uint64_t>& state);
#ifndef _WIN32
    /** 以独立参数异步交给桌面程序，回收直接子进程。
    @param [in] program 系统程序路径或 PATH 中的程序名
    @param [in] option 可空的第一个选项
    @param [in] path 单独传入的文件或目录
    @note 支持平台 Linux、macOS
    */
    static void openDesktopPath(const std::string& program, const char* option, const std::string& path);
#endif
#endif
#if CSYSTEM_HAS_NATIVE_API
    /** 取得当前可执行文件路径中的指定部分，Windows 使用系统窄字符编码
    @param [in] flag 1 取全名，3 去掉扩展名，4 取目录并保留末尾分隔符
    @return 指定路径部分，查询失败返回空串
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string executablePathPart(int32_t flag);

    /** 判断存在路径是普通目录，不接受链接或重解析点
    @param [in] path 待检查目录
    @return 普通无链接目录返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool plainDirectory(const std::string& path);

    /** 判断存在路径是普通文件，不接受链接或重解析点
    @param [in] path 待检查文件
    @return 普通无链接文件返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool plainFile(const std::string& path);

    /** 以固定缓冲逐字节比较两个文件
    @param [in] first 第一文件
    @param [in] second 第二文件
    @return 两个文件完整读取且全部字节相同返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool equalFiles(const std::string& first, const std::string& second);

    /** 保留迁移文件时间，使依赖修改时间的内容版本保持稳定
    @param [in] source 已验证源文件
    @param [in] destination 已复制的目标文件
    @return 时间读取及写入成功返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool copyFileMetadata(const std::string& source, const std::string& destination);

    /** 复制到目标同目录临时文件，核对后无覆盖提交，再删源文件
    @param [in] source 普通源文件，调用方保证迁移期间没有并发写入
    @param [in] destination 目标路径
    @param [out] errorText 错误原因
    @return 目标字节完整、源移除成功返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool migrateFile(const std::string& source, const std::string& destination, std::string& errorText);

    /** 迁移已经核对为非重叠的目录树
    @param [in] source 普通源目录，调用方保证迁移期间没有并发写入
    @param [in] destination 普通目标目录
    @param [out] errorText 错误原因
    @return 所有子项完成且源空目录移除返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool migrateTree(const std::string& source, const std::string& destination, std::string& errorText);

#ifdef _WIN32

    /** 枚举本次请求的可见窗口，找到后停止
    @param [in] window 当前顶层窗口
    @param [in,out] context 同步调用期间有效的进程ID与结果对
    @return 继续枚举返回TRUE，匹配后返回FALSE
    @note 支持平台 Windows
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
    @note 支持平台 Windows
    */
    static uint32_t startOwnedProcess(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& output, uint32_t& pid, uint64_t& monitor);
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 按单字符分隔并保留空字段，先清空输出数组
    @param [out] result 本次切分结果
    @param [in] splitString 待切分字节串
    @param [in] separateCharacter 单字节分隔符
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void split(std::vector<std::string>& result, const std::string& splitString, char separateCharacter);

    /** 向可选错误输出写入固定错误文本
    @param [out] errorText 可空，非空时替换内容
    @param [in] value 本次错误文本
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static void setSystemError(std::string* errorText, const std::string& value);

    /** 将系统路径宽字符串转换为UTF8，保留内嵌NUL以供调用入口校验
    @param [in] text 借用至返回的宽字符串，允许为空
    @return 转换结果；空输入、转换或分配失败返回空串，Windows沿用系统flags=0的替代字符规则
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::string widePathToUtf8(const std::wstring& text);

    /** 将UTF8路径转换为系统宽字符串，保留内嵌NUL以供调用入口校验
    @param [in] text 借用至返回的UTF8字节串，允许为空
    @return 转换结果；空输入、转换或分配失败返回空串，Windows沿用系统flags=0的替代字符规则
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static std::wstring utf8PathToWide(const std::string& text);

#if CSYSTEM_HAS_DESKTOP_API
    /** 在控制台读取一个不回显的字符，POSIX读取后恢复原终端设置
    @return 字符值；标准输入到达末尾时返回EOF
    @note 支持平台 Windows、Linux、macOS
    */
    static int getConsoleCharacter();
#endif // CSYSTEM_PLATFORM_CAPABILITY

    /** 保存文件操作错误并统一返回失败
    @param [out] error 可空，非空时写入系统错误码
    @param [in] value Win32错误码或errno
    @return false
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool systemFileFailure(uint32_t* error, uint32_t value);

    /** 检查原生文件句柄的数值范围，避免POSIX宽整数被截断为另一个描述符
    @param [in] handle Windows句柄或POSIX文件描述符
    @return 数值可表示且不为无效句柄时返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool isSystemFileHandle(intptr_t handle);

    /** 检查句柄指向可寻址普通文件，拒绝目录及重解析点
    @param [in] handle 已打开且数值已校验的句柄
    @param [out] error 可空，失败写入系统错误码
    @return 是普通文件时返回true
    @note 支持平台 Windows、Linux、macOS、Android、iOS
    */
    static bool validateSystemFileHandle(intptr_t handle, uint32_t* error);

#ifdef _WIN32

    /** 为当前模块申请旧版 Windows 的毫秒计时精度，模块卸载时归还
    @param [in] once 系统的一次性初始化状态，本回调不保存
    @param [in] parameter 未使用
    @param [out] context 未使用，不写入
    @return 初始化完成或系统不提供该能力时返回 TRUE，发生异常时返回 FALSE 允许下次重试
    @note 支持平台 Windows
    */
    static BOOL CALLBACK initializeTimerResolution(PINIT_ONCE once, PVOID parameter, PVOID* context);

    /** 使用普通 Windows 计时器等待，首次非零等待时尝试申请 1 毫秒计时精度
    @param [in] milliseconds 等待时长，单位毫秒
    @note 支持平台 Windows
    */
    static void sleepWithTimerResolution(uint32_t milliseconds);

    /** 枚举目标进程的第一个顶层窗口，匹配后停止，不限制窗口可见性
    @param [in] window 系统回调提供的窗口
    @param [in,out] context 同步借用的进程ID与窗口结果对
    @return 匹配时FALSE，否则TRUE继续枚举
    @note 支持平台 Windows
    */
    static BOOL CALLBACK findWindowCallback(HWND window, LPARAM context);

    /** 兼容旧Windows动态取得真实本机系统信息
    @param [out] information 可空，非空时写入系统信息；缺少新入口时使用GetSystemInfo
    @note 支持平台 Windows
    */
    static void getNativeSystemInfo(LPSYSTEM_INFO information);
#endif

#ifdef _MSC_VER

    /** 枚举指定DirectShow设备类别
    @param [in] category 系统设备类别标识
    @param [out] enumeration 不可空，成功后调用方负责Release
    @return 成功S_OK；空输出E_POINTER；没有设备VFW_E_NOT_FOUND；其他情况返回COM错误
    @note 支持平台 Windows
    */
    static HRESULT enumerateDevices(REFGUID category, IEnumMoniker** enumeration);

    /** 从DirectShow枚举器取得设备名称并追加到结果
    @param [in] enumeration 可空，借用至返回，不释放枚举器
    @param [in,out] names 设备名称列表，已有内容保留
    @note 支持平台 Windows
    */
    static void displayDeviceInformation(IEnumMoniker* enumeration, std::vector<std::string>& names);

#endif

#if defined(__linux__)

    /** 从同一次proc stat快照确认整个线程组退出
    @param [in] line proc stat完整内容，进程名允许空格及括号
    @return 仅Z或X状态且字段20明确为1时true；缺字段或还有其他线程时false
    @note 支持平台 Linux、Android
    */
    static bool linuxProcessStatExited(const std::string& line);
#endif

#ifdef CSYSTEM_SHARED_IDENTITY

    /** 取得本进程启动期配置的身份登记目录
    @return 进程内配置引用，仅允许setProcessDirectory在首次登记前串行修改
    @note 支持平台 Windows、Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static std::string& processDirectory();

    /** 取得身份登记目录的封存标记，首次发布尝试后禁止更换路径
    @return 进程内原子标记引用，不承担目录字符串的并发修改同步
    @note 支持平台 Windows、Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static std::atomic<bool>& processDirectorySealed();

    /** 一次性生成并发布本进程的随机实例身份，初始化失败可重试
    @return 本进程非零64位身份；随机源、目录或登记失败返回0
    @note 支持平台 Windows、Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static uint64_t mobileSelfIdentity();

#ifdef _WIN32
    /** 以 Windows InitOnce 发布共享目录后端的身份，失败时允许重试
    @param [in] once 系统初始化状态，不保存
    @param [out] parameter 不可空，指向进程级64位身份存储，仅完整发布后写入
    @param [out] context 未使用，不写入
    @return 完整发布返回TRUE；失败返回FALSE允许后续重试，不向系统回调传播异常
    @note 支持平台 Windows（需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static BOOL CALLBACK initializeProcessIdentity(PINIT_ONCE once, PVOID parameter, PVOID* context);

#endif

    /** 通过同目录临时文件原子替换发布完整身份，不使用共享映射或跨进程文件锁
    @param [in] identity 非零随机64位身份
    @return 完整记录已发布时true；失败清理本次临时文件并保留原记录
    @note 支持平台 Windows、Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static bool publishProcessIdentity(uint64_t identity);

    /** 读取参与进程的完整身份登记，不创建缺失记录
    @param [in] processId 已校验且尚未明确退出的正进程号
    @param [out] identity 成功写入非零身份，失败置0
    @return 取得完整有效记录时true；缺失、类型错误或读取失败为false
    @note 支持平台 Windows、Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static bool readProcessIdentity(uint32_t processId, uint64_t& identity);

#ifndef _WIN32

    /** 打开宿主已有身份登记目录，拒绝末级链接及其他UID拥有的目录
    @param [in] directory 不含NUL的绝对路径
    @return 目录文件描述符，调用方负责close；非法或系统失败返回-1
    @note 支持平台 Linux、macOS、Android、iOS（移动端默认启用，桌面端需定义PROCESS_WORK_PORTABLE_IDENTITY）
    */
    static int openProcessDirectory(const std::string& directory);
#endif
#endif
#endif // CSYSTEM_NATIVE_API
};

#include "CSystem.inl"