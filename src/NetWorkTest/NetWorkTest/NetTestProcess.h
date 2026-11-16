#pragma once
#include <cstdint>
#include <string>
#include <vector>

// 测试子进程的唯一拥有者；不借用业务进程，不按进程名结束程序
class NetTestProcess
{
public:
    /** 创建未启动的进程观察器
    */
    NetTestProcess();

    /** 回收本对象创建的进程及其输出句柄，未退出时先终止；Windows同时回收作业内后代
    */
    ~NetTestProcess();

    /** 不经shell启动一次测试子进程；只允许未使用的对象
    @param [in] executable 绝对可执行路径
    @param [in] arguments 独立原始参数，不预先添加shell引号
    @param [in] logFile 不存在的绝对日志路径，父目录已创建
    @param [in] normalStartup Windows不强制隐藏或禁用控制台，用于检查GUI程序自身的启动行为
    @return 启动及所有权登记成功返回true；失败不保留运行进程
    */
    bool start(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& logFile, bool normalStartup = false);

    /** 查询本次Windows作业内仍存活的指定映像，不按全局进程名推断测试所有权
    @param [in] executable 候选映像绝对路径
    @param [out] processes 本次作业内匹配的存活PID，只用于观察
    @return 成功完成作业和映像查询返回true；非Windows返回false
    */
    bool ownedProcesses(const std::string& executable, std::vector<uint32_t>& processes) const;

    /** 在同一个内核句柄上核对作业归属和映像，再模拟本测试后代崩溃
    @param [in] process 由ownedProcesses观察的PID
    @param [in] executable 必须匹配的候选映像绝对路径
    @return 确认归属、终止并等待退出成功返回true
    */
    bool terminateOwned(uint32_t process, const std::string& executable) const;

    /** 查询本对象创建的进程；退出后重复查询保持同一结果
    @param [out] exitCode 正常退出码或负的POSIX信号编号
    @return 1已经退出，0仍运行，-1观察失败
    */
    int32_t poll(int32_t& exitCode);

    /** 等待真实退出，超时不会当作成功或自动丢弃进程
    @param [in] timeoutMs 总期限，0只检查一次
    @param [out] exitCode 仅返回1时有效
    @return 1已经退出，0超过期限，-1观察失败
    */
    int32_t wait(uint32_t timeoutMs, int32_t& exitCode);

    /** 等待Windows测试作业全部退出，不能把父进程退出当作整棵进程树已清理
    @param [in] timeoutMs 等待作业内存活进程归零的总期限
    @return 1全部退出，0仍有后代，-1未启动、查询失败或平台不支持作业观察
    */
    int32_t waitTree(uint32_t timeoutMs) const;

    /** 终止并回收仅本对象创建的子进程；正常已退出时不重复结束PID
    @return 进程已退出且被回收返回true
    */
    bool terminate();

    /** 读取本次日志；超过预算或读取失败时保留原输出
    @param [out] text UTF8日志，不解释测试成功标记
    @return 完整读取且不超过16MiB返回true
    */
    bool output(std::string& text) const;

    /** 查询启动时记录的进程号，用于核对就绪日志身份
    @return 0表示尚未成功启动
    */
    uint32_t pid() const;

private:
    /** Windows在恢复主线程前绑定测试作业，保证超时后的后代进程也有明确所有者
    @param [in] executable 绝对可执行路径
    @param [in] arguments 独立UTF8参数
    @param [in] logFile 本次新日志路径
    @param [in] normalStartup 不覆盖被测程序的窗口和控制台行为
    @return 全部句柄及作业建立完成并已恢复运行返回true
    */
    bool startWindows(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& logFile, bool normalStartup);

private:
    /** 禁止复制进程及句柄所有权
    @param [in] other 不允许复制的观察器
    */
    NetTestProcess(const NetTestProcess& other);

    /** 禁止重复拥有同一子进程
    @param [in] other 不允许赋值的观察器
    @return 不提供实现
    */
    NetTestProcess& operator=(const NetTestProcess& other);

private:
    // Windows为本对象创建并保留的进程内核句柄；其他平台不用
    uint64_t m_monitor;
    // Windows专用测试作业；关闭时仅终止该作业继承树中的进程
    uint64_t m_job;
    // 仅本对象启动的PID；POSIX在waitpid回收之前不允许被系统复用
    uint32_t m_pid;
    // 已观察并缓存退出事实，不再次查询可能复用的POSIX PID
    bool m_exited;
    // 缓存的真实退出状态
    int32_t m_exitCode;
    // 本次独占创建的输出文件
    std::string m_logFile;
};