#pragma once
#include "LogManagerConfig.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <stdint.h>
#include <string>

/** 单个日志文件ID对应的运行时状态，记录当前实体文件、软链接、滚动序号和删除保护句柄
*/
class LogManagerFile
{
public:
    /** 构造函数
    */
    LogManagerFile();

    /** 析构函数，只做兜底资源释放
    */
    ~LogManagerFile();

public:
    /** 独占创建下一分卷，失败时保留当前分卷
    @param [in] path 新分卷路径
    @return 新分卷可写时返回 true
    */
    bool openEntity(const std::string& path);

    /** 释放当前分卷句柄，可重复调用
    */
    void closeEntity();

    /** 预留独立区间，刷新正文后提交标记并再次刷新
    @param [in] line 已格式化正文
    @return 两次刷新均成功时返回 true
    */
    bool append(const std::string& line);

public:
    // 日志初始化配置
    LogManagerConfig m_config;
    // 初始化时固定的绝对日志目录，带末尾路径分隔符
    std::string m_logDir;
    // 本次会话的唯一日志基础名，同秒重名时追加序号
    std::string m_baseName;
    // 本次会话的归档目录，初始化时确定，关闭时才创建
    std::string m_archiveDir;
    // 本次会话成功创建且尚未滚删的实体日志路径
    std::deque<std::string> m_entityPaths;
    // 本会话创建的软链接所指卷号，-1 表示没有自有链接；更新受阻时保留原卷号
    int32_t m_linkIndex;
    // 当前日志软链接路径
    std::string m_linkPath;
    // 当前实体日志路径
    std::string m_currentEntityPath;
    // 当前实体文件下标
    int32_t m_currentIndex;
    // 达到容量和检查周期后由下个独占写入者换卷
    std::atomic<bool> m_rollRequested;
    // 当前软链接删除保护句柄
    void* m_linkFileLock;

private:
#ifdef _WIN32
    // 当前可写的 Windows 文件句柄
    void* m_file;
    // 映射偏移的系统对齐单位，字节
    uint32_t m_granularity;
#else
    // 当前分卷描述符，-1 表示已关闭
    int m_fd;
#endif
    // 本卷下一条记录可预留的字节位置
    std::atomic<int64_t> m_tail;
    // 本卷已尝试写入的记录数，用于决定检查时机
    std::atomic<uint64_t> m_written;

private:
    /** 将记录写入预留区间，先刷新正文再提交标志
    @param [in] start 预留区间起始字节
    @param [in] record 含前导换行与临时标志的完整字节串
    @return 两次刷新均成功时返回 true
    */
    bool commit(int64_t start, const std::string& record);
#ifndef _WIN32
    /** 完整写入指定偏移，处理短写和中断
    @param [in] offset 文件字节偏移
    @param [in] data 本次写入的缓冲区
    @param [in] size 缓冲区字节数
    @return 全部写入成功时返回 true
    */
    bool writeAt(int64_t offset, const char* data, size_t size);
#endif
};

/** 快速写入期间持有注册表和文件句柄的使用权 */
class LogManagerWriterGuard
{
public:
    /** 尝试进入写入区，管理操作正在等待时不进入
    @param [in,out] state 管理器拥有的写者计数及关闭位
    */
    explicit LogManagerWriterGuard(std::atomic<uint32_t>& state);

    /** 归还已取得的使用权 */
    ~LogManagerWriterGuard();

    /** 查询是否可以读取注册表
    @return 当前对象取得使用权时返回 true
    */
    bool acquired() const;

    // 该位关闭新写者入口，低位继续记录仍在写入的线程
    static const uint32_t kClosing = uint32_t(1) << 31;

private:
    /** 禁止复制写者使用权
    @param [in] other 原使用权
    */
    LogManagerWriterGuard(const LogManagerWriterGuard& other) = delete;

    // 借用管理器计数，管理器在本对象析构前有效
    std::atomic<uint32_t>& m_state;
    // 是否已增加活跃写者计数
    bool m_acquired;
};

/** 排他操作先阻止新写者，再等在途写入完成 */
class LogManagerRegistryGuard
{
public:
    /** 取得管理操作的独占使用权
    @param [in,out] mutex 管理操作共用的互斥量
    @param [in,out] state 同一管理器的写者计数
    */
    LogManagerRegistryGuard(std::mutex& mutex, std::atomic<uint32_t>& state);

    /** 恢复写者入口，再释放互斥量 */
    ~LogManagerRegistryGuard();

private:
    // 保证同一时刻只有一个操作可以关闭写者入口
    std::lock_guard<std::mutex> m_lock;
    // 借用至排他操作结束的计数
    std::atomic<uint32_t>& m_state;
};