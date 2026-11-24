#pragma once
#include "LogManagerConfig.h"
#include "LogManagerMacro.h"
#include <array>
#include <atomic>
#include <map>
#include <mutex>
#include <stdint.h>
#include <string>

// 输出到默认日志文件，使用方式：LOGINFO("user=%s", name.c_str())，其中LOGDEBUG在Release下不会写入日志
#define LOGDEBUG(format, ...) LogManager::instance().print(0, LogManager::LOG_DEBUG, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGINFO(format, ...) LogManager::instance().print(0, LogManager::LOG_INFO, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGWARNING(format, ...) LogManager::instance().print(0, LogManager::LOG_WARNING, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGERROR(format, ...) LogManager::instance().print(0, LogManager::LOG_ERROR, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGFATAL(format, ...) LogManager::instance().print(0, LogManager::LOG_FATAL, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

// 输出到指定日志文件ID，使用方式：LOGINFO_EX(fileId, "user=%s", name.c_str())
#define LOGDEBUG_EX(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_DEBUG, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGINFO_EX(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_INFO, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGWARNING_EX(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_WARNING, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGERROR_EX(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_ERROR, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGFATAL_EX(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_FATAL, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

// 输出到默认日志文件并按调用次数采样，使用方式：LOGINFO_EVERY(1000, "value=%d", value)，第一次会输出，之后每隔interval次再输出
#define LOGDEBUG_EVERY(interval, format, ...) LOGMANAGER_PRINT_EVERY(0, LogManager::LOG_DEBUG, interval, format, ##__VA_ARGS__)
#define LOGINFO_EVERY(interval, format, ...) LOGMANAGER_PRINT_EVERY(0, LogManager::LOG_INFO, interval, format, ##__VA_ARGS__)
#define LOGWARNING_EVERY(interval, format, ...) LOGMANAGER_PRINT_EVERY(0, LogManager::LOG_WARNING, interval, format, ##__VA_ARGS__)
#define LOGERROR_EVERY(interval, format, ...) LOGMANAGER_PRINT_EVERY(0, LogManager::LOG_ERROR, interval, format, ##__VA_ARGS__)
#define LOGFATAL_EVERY(interval, format, ...) LOGMANAGER_PRINT_EVERY(0, LogManager::LOG_FATAL, interval, format, ##__VA_ARGS__)

// 输出到指定日志文件ID并按调用次数采样，使用方式：LOGINFO_EX_EVERY(fileId, 1000, "value=%d", value)
#define LOGDEBUG_EX_EVERY(fileId, interval, format, ...) LOGMANAGER_PRINT_EVERY(fileId, LogManager::LOG_DEBUG, interval, format, ##__VA_ARGS__)
#define LOGINFO_EX_EVERY(fileId, interval, format, ...) LOGMANAGER_PRINT_EVERY(fileId, LogManager::LOG_INFO, interval, format, ##__VA_ARGS__)
#define LOGWARNING_EX_EVERY(fileId, interval, format, ...) LOGMANAGER_PRINT_EVERY(fileId, LogManager::LOG_WARNING, interval, format, ##__VA_ARGS__)
#define LOGERROR_EX_EVERY(fileId, interval, format, ...) LOGMANAGER_PRINT_EVERY(fileId, LogManager::LOG_ERROR, interval, format, ##__VA_ARGS__)
#define LOGFATAL_EX_EVERY(fileId, interval, format, ...) LOGMANAGER_PRINT_EVERY(fileId, LogManager::LOG_FATAL, interval, format, ##__VA_ARGS__)

// 输出到默认日志文件且同一个调用点只输出第一次，使用方式：LOGINFO_ONCE("only once")
#define LOGDEBUG_ONCE(format, ...) LOGDEBUG_EVERY(-1, format, ##__VA_ARGS__)
#define LOGINFO_ONCE(format, ...) LOGINFO_EVERY(-1, format, ##__VA_ARGS__)
#define LOGWARNING_ONCE(format, ...) LOGWARNING_EVERY(-1, format, ##__VA_ARGS__)
#define LOGERROR_ONCE(format, ...) LOGERROR_EVERY(-1, format, ##__VA_ARGS__)
#define LOGFATAL_ONCE(format, ...) LOGFATAL_EVERY(-1, format, ##__VA_ARGS__)

// 输出到指定日志文件ID且同一个调用点只输出第一次，使用方式：LOGINFO_EX_ONCE(fileId, "only once")
#define LOGDEBUG_EX_ONCE(fileId, format, ...) LOGDEBUG_EX_EVERY(fileId, -1, format, ##__VA_ARGS__)
#define LOGINFO_EX_ONCE(fileId, format, ...) LOGINFO_EX_EVERY(fileId, -1, format, ##__VA_ARGS__)
#define LOGWARNING_EX_ONCE(fileId, format, ...) LOGWARNING_EX_EVERY(fileId, -1, format, ##__VA_ARGS__)
#define LOGERROR_EX_ONCE(fileId, format, ...) LOGERROR_EX_EVERY(fileId, -1, format, ##__VA_ARGS__)
#define LOGFATAL_EX_ONCE(fileId, format, ...) LOGFATAL_EX_EVERY(fileId, -1, format, ##__VA_ARGS__)

// 每个调用点的无参 lambda 返回自己的原子序号；列表初始化保证先取序号，再求间隔。
// 判定 lambda 接收这两个数并返回是否命中，调用方的 ID、间隔和消息实参均在原作用域求值。
#define LOGMANAGER_PRINT_EVERY(fileId, level, interval, format, ...) \
    do \
    { \
        if ([](const std::array<int64_t, 2>& sample) -> bool \
            { \
                return ::LogManager::needPrintEvery(sample[0], sample[1]); \
            }(std::array<int64_t, 2>{{[]() -> int64_t \
                { \
                    static std::atomic_llong s_calls = ATOMIC_VAR_INIT(0); \
                    return s_calls.fetch_add(1); \
                }(), static_cast<int64_t>(interval)}})) \
        { \
            LogManager::instance().print((fileId), (level), __FILE__, __FUNCTION__, "", "", 0, (format), ##__VA_ARGS__); \
        } \
    } while (false)

// BEGIN和END日志为LogManager内部使用，业务代码不要直接调用
#define LOGBEGIN(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_BEGIN, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGEND(fileId, format, ...) LogManager::instance().print(fileId, LogManager::LOG_END, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

class LogManagerFile;

/** 日志管理器，独占本进程的日志会话，负责同步写入日志、按大小滚动文件、维护当前日志软链接，并提供按调用次数采样输出的宏接口
*/
class LogManagerAPI LogManager
{
public:
    /** 日志级别
    */
    enum LogLevel
    {
        // 调试日志，Release配置下不会输出
        LOG_DEBUG = 0x00000001,
        // 普通信息日志
        LOG_INFO = 0x00000010,
        // 警告日志
        LOG_WARNING = 0x00000100,
        // 错误日志
        LOG_ERROR = 0x00001000,
        // 严重错误日志
        LOG_FATAL = 0x00010000,
        // 日志文件开始标记，内部使用
        LOG_BEGIN = 0x00100000,
        // 日志文件结束标记，内部使用
        LOG_END = 0x01000000
    };

private:
    /** 构造函数，真正的日志文件初始化由init完成
    */
    LogManager();

    /** 析构函数，会关闭所有已经打开的日志文件
    */
    ~LogManager();

private:
    /** 禁止拷贝构造，日志管理器只能通过instance访问
    @param [in] other 被拷贝对象
    */
    LogManager(const LogManager& other);

    /** 禁止赋值，日志管理器只能通过instance访问
    @param [in] other 被赋值对象
    @return 返回当前对象
    */
    LogManager& operator=(const LogManager& other);

public:
    /** 获取日志管理器单例
    @return 返回日志管理器单例
    */
    static LogManager& instance();

    /** 判断按次数输出的调用点本次是否应该真正写日志
    @param [in] callIndex 当前调用点从0开始的调用次数
    @param [in] interval 间隔次数，0表示每次写，负数表示只写第一次，正数表示只写调用序号为 interval 倍数的项
    @return 返回本次是否需要写日志
    */
    static bool needPrintEvery(int64_t callIndex, int64_t interval);

public:
    /** 初始化一个日志文件
    @param [in] config 日志配置，配置中m_fileId用于区分多个日志文件
    */
    void init(const LogManagerConfig& config);

    /** 使用最近一次预设配置初始化，未设置时使用内置默认值
    */
    void init();

    /** 复制下一次无参初始化的配置，已有会话继续使用创建时的配置
    @param [in] config 所有字段均可在初始化前设置；默认宏延迟打开的 ID 固定为 0
    */
    void setDefaultConfig(const LogManagerConfig& config);

    /** 动态开关日志输出和BEGIN/END输出
    @param [in] writeLog 是否写日志
    @param [in] writeBeginEnd 是否写BEGIN和END标记
    */
    void set(bool writeLog, bool writeBeginEnd);

    /** 同步写一条日志。记录以换行和[0]开头，正文刷新后改写[1]并再次刷新。
    持久化失败通过标准错误输出。Wasm 需要有持久化刷新能力的宿主文件系统。
    @param [in] fileId 日志文件ID
    @param [in] flag 日志级别
    @param [in] fileMacro 调用点的__FILE__
    @param [in] funName 调用点的__FUNCTION__
    @param [in] exeName 自定义进程名，空字符串表示使用当前进程名
    @param [in] intDateTime 自定义时间文本，空字符串表示使用当前时间
    @param [in] threadId 自定义线程 ID，以无分组的无符号十进制写入；0 在原生端取当前线程 ID，Wasm 保留 0
    @param [in] format printf风格格式串
    */
    void print(int32_t fileId, LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, ...);

    /** 关闭一个日志文件，并按配置归档本次会话自己创建的文件
    @param [in] fileId 日志文件ID
    */
    void uninit(int32_t fileId);

    /** 关闭所有日志文件，并按各自配置归档；不会处理其他进程或崩溃遗留文件
    */
    void uninitAll();

    /** 关闭并删除指定日志文件当前会话生成的所有实体文件和软链接
    @param [in] fileId 日志文件ID
    */
    void deleteFile(int32_t fileId);

    /** 根据日志路径查找文件ID
    @param [in] path 日志目录、当前实体文件路径或软链接路径；相对查询使用调用时的工作目录
    @return 返回日志文件ID，找不到返回-1
    */
    int32_t findFileId(const std::string& path);

private:
    /** 获取日志文件状态
    @param [in] fileId 日志文件ID
    @return 返回日志文件状态，找不到返回nullptr
    */
    LogManagerFile* getLogFile(int32_t fileId);

private:
    /** 在已经持有写锁时初始化日志文件
    @param [in] config 日志配置
    @return 返回日志文件状态，初始化失败返回nullptr
    */
    LogManagerFile* initNoLock(const LogManagerConfig& config);

    /** 在已经持有写锁时关闭日志文件
    @param [in] fileId 日志文件ID
    */
    void uninitNoLock(int32_t fileId);

    /** 在已经持有写锁时写日志行
    @param [in] logFile 日志文件状态
    @param [in] line 已经格式化好的日志行
    */
    void writeLineNoLock(LogManagerFile* logFile, const std::string& line);

    /** 在写者租期内写入并同步提交
    @param [in] logFile 在整个调用期间存活的分卷
    @param [in] line 已格式化的正文
    */
    void appendLine(LogManagerFile* logFile, const std::string& line);

    /** 跳过已占用的卷号，排他新建下一卷；其他打开错误时保留当前卷
    @param [in,out] logFile 已取得独占管理权的会话
    */
    void rollNoLock(LogManagerFile* logFile);

    /** 在已经持有写锁时打开指定下标的实体日志文件
    @param [in,out] logFile 日志文件状态
    @param [in] fileIndex 实体日志文件下标
    @param [in] entityPath 此下标对应的新卷绝对路径
    @return 返回是否打开成功
    */
    bool openLogFileNoLock(LogManagerFile* logFile, int32_t fileIndex, const std::string& entityPath);

    /** 在独占管理期间更新软链接，旧链接暂时无法删除时保留其卷号供重试
    @param [in] logFile 日志文件状态
    */
    void updateLinkNoLock(LogManagerFile* logFile);

    /** 在已经持有写锁时为本次会话排他打开首卷并确定会话归档路径
    @param [in,out] logFile 日志文件状态
    @return 首卷创建成功时返回 true
    */
    bool openSessionNoLock(LogManagerFile* logFile);

    /** 文件和保护锁关闭后，仅归档本次会话记录的文件；失败保留源文件
    @param [in] logFile 日志文件状态
    */
    void archiveSessionNoLock(LogManagerFile* logFile);

    /** 在已经持有写锁时删除超过保留数量的旧滚动文件
    @param [in] logFile 日志文件状态
    */
    void deleteExpiredLogNoLock(LogManagerFile* logFile);

    /** 根据已经格式化好的消息正文生成完整日志行
    @param [in] flag 日志级别
    @param [in] fileMacro 调用点的__FILE__
    @param [in] funName 调用点的__FUNCTION__
    @param [in] exeName 自定义进程名
    @param [in] intDateTime 自定义时间文本
    @param [in] threadId 自定义线程ID
    @param [in] message 已经格式化好的消息正文
    @return 返回完整日志行，日志级别被过滤时返回空字符串
    */
    std::string formatLineText(LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const std::string& message);

private:
    // 当前进程文件名，带后缀，用于日志行中显示
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_exeName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前进程名，不带后缀，用于生成日志文件名
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::string m_processName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 日志文件ID到文件状态的映射
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::map<int32_t, LogManagerFile*> m_logMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 是否写BEGIN和END标记
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::atomic<bool> m_writeBeginEnd;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 是否写日志
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::atomic<bool> m_writeLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 后续无参初始化所用的配置，由管理互斥量保护
    LogManagerConfig m_defaultConfig;
    // 串行管理操作和控制台写入
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::mutex m_management;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 最高位禁止新写者进入，其余位为正在使用注册表和分卷的写者数
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::atomic<uint32_t> m_activeWriters;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};