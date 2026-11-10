#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>

/** 单个黑盒用例的注册信息，回调和名称在测试运行期间有效
*/
struct BlackBoxCase
{
    // 命令行选择用的稳定用例编号
    int id;
    // 报告中的用例名称，借用静态字符串
    const char* name;
    // 预期行为说明，借用静态字符串
    const char* expectation;
    // 是否为耗时压力测试
    bool stress;
    // 子进程允许执行的最长秒数
    int timeoutSeconds;
    // 在隔离子进程中执行的用例体，异常由测试入口记录
    std::function<void()> run;
    // 不适用组合的原因，空指针表示正常运行
    const char* unavailableReason;
};

/** 拥有一个测试子进程及其进程组，析构清理仍在运行的自有进程
*/
class BlackBoxChild
{
public:
    /** 创建尚未启动的子进程所有者
    */
    BlackBoxChild();

    /** 关闭自有句柄，终止尚未退出的自有测试进程
    */
    ~BlackBoxChild();

    /** 启动当前测试程序的隔离用例，不经过命令解释器
    @param [in] arguments 传给子进程的命令行参数
    @param [in] logPath 子进程输出日志路径
    */
    void start(const std::vector<std::string>& arguments, const std::string& logPath);

    /** 启动明确指定的测试或编译子进程，所有权和超时清理与用例相同
    @param [in] executable 已存在的可执行程序路径
    @param [in] arguments 独立参数，不经过命令解释器
    @param [in] logPath 子进程输出路径
    */
    void startProgram(const std::string& executable, const std::vector<std::string>& arguments,
        const std::string& logPath);

    /** 只终止本对象启动且仍在运行的子进程
    @param [in] exitCode Windows 下使用的退出码
    */
    void stop(int32_t exitCode = 124);

    /** 非阻塞查询子进程状态
    @return -1 为仍在运行，其他值为退出码
    */
    int poll();

    /** 有时限等待子进程，超时抛测试失败异常
    @param [in] milliseconds 最长等待毫秒数
    @return 已退出子进程的退出码
    */
    int wait(int milliseconds);

    /** 查询启动时记录的进程编号
    @return 未启动为 0
    */
    int pid() const;

private:
    /** 禁止复制进程所有权
    @param [in] other 不允许复制的来源
    */
    BlackBoxChild(const BlackBoxChild& other);

    /** 禁止覆盖已有进程所有权
    @param [in] other 不允许复制的来源
    @return 无可调用实现
    */
    BlackBoxChild& operator=(const BlackBoxChild& other);

private:
    // Windows 进程句柄，由本对象关闭
    void* m_process;
    // Windows 作业对象句柄，包含本测试创建的后代进程
    void* m_job;
    // 子进程编号，未启动为 0
    int m_pid;
    // 缓存的退出码，-1 表示尚未退出
    int m_result;
    // POSIX 下是否拥有独立进程组
    bool m_processGroup;
};

/** 条件不成立时抛出测试失败异常
@param [in] condition 预期为真的条件
@param [in] message 失败说明
*/
void bbCheck(bool condition, const std::string& message);

/** 记录析构清理失败，不从析构函数向外抛异常
@param [in] message 非空失败说明
*/
void bbCleanupFailure(const char* message);

/** 检查当前子进程是否记录过清理失败
*/
void bbCheckCleanup();

/** 轮询到条件成立，超时抛出带说明的测试失败异常
@param [in] predicate 同步执行的判断函数
@param [in] description 非空等待说明
@param [in] milliseconds 最长等待毫秒数
*/
void bbWait(const std::function<bool()>& predicate, const char* description, int milliseconds = 5000);

/** 暂停当前测试线程
@param [in] milliseconds 非负等待毫秒数
*/
void bbSleep(int milliseconds);

/** 取得测试超时计算使用的单调时钟
@return 毫秒计数，只用于同进程差值
*/
uint64_t bbNow();

/** 查询当前测试进程编号
@return 操作系统分配的进程编号
*/
int bbPid();

/** 将整数转换成测试日志中的十进制字符串
@param [in] number 待转换数值
@return 十进制表示
*/
std::string bbNumber(int64_t number);

/** 查询当前测试程序的绝对路径
@return 可用于启动隔离子进程的路径
*/
std::string bbExecutable();

/** 生成本测试进程独立的临时文件路径，不创建文件
@param [in] suffix 文件名后缀
@return 含进程编号、时间和递增序号的路径
*/
std::string bbTempPath(const std::string& suffix);

/** 读取测试文件，无法打开时返回空内容
@param [in] path 文件路径
@return 文件字节内容
*/
std::string bbRead(const std::string& path);

/** 覆盖写入测试文件，打开或写入失败时抛测试异常
@param [in] path 文件路径
@param [in] contents 完整字节内容
*/
void bbWrite(const std::string& path, const std::string& contents);

/** 执行用例选择、子进程隔离和报告输出
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数，借用至函数返回
@param [in] suite 非空测试套件名称
@param [in] cases 本次可用的用例注册表
@return 测试程序退出码，0 表示所选可运行用例通过
*/
int bbRun(int argc, char** argv, const char* suite, const std::vector<BlackBoxCase>& cases);