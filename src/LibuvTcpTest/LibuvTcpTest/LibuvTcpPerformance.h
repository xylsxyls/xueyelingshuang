#pragma once
#include "uv.h"
#include <string>
#include <vector>

/** 测试工程独占的子进程及性能批次执行器，不向生产库注入代码
*/
class LibuvTcpPerformance
{
public:
    /** 创建空的子进程运行上下文
    */
    LibuvTcpPerformance();

    /** 执行28个稳定场景，每版一次预热，交替运行并输出原始样本及中位数CSV
    @param [in] argc 参数数量
    @param [in] argv --suite 正式轮数 基准EXE或- 可选场景编号；编号0表示全部
    @return 0所选样本全部有效，非零参数、子进程、超时或数据错误
    */
    static int run(int argc, char** argv);

    /** 测试CSV解析和坏样本拒绝，不启动网络或子进程
    @return 有效、截断、非法数值及交付数量检查符合预期
    */
    static bool checkParser();

    /** 验证独立测试子进程的成功、非零退出、缺失、超时和输出越界收敛
    @return 所有样本按预期接受或拒绝，句柄均完成回收
    */
    static bool checkRunner();

    /** 批次执行器的隔离故障子进程，仅供测试器自检，不进入生产代码
    @param [in] mode 0成功、1非零退出、2过量输出、3超时
    @return 被构造的进程状态
    */
    static int runnerProbe(const char* mode);

private:
    /** 启动一个确切程序，继承亲和性；最多120秒且最多收集64KiB输出
    @param [in] executable 目标EXE完整路径
    @param [in] arguments 四个数值及测试模式
    @param [out] output 完整标准输出
    @param [in] milliseconds 截止时限，自检使用短时限，其余默认120秒
    @return 子进程正常退出且没有超时或采集失败
    */
    bool execute(const char* executable, const std::vector<std::string>& arguments, std::string& output,
        uint64_t milliseconds = 120000);

    /** 分析最后一行完整CSV并验证实际交付；未知指标不填零
    @param [in] output 子进程输出
    @param [in] kind 0六字节、1延迟、2原始字节
    @param [in] expected 六字节组预期交付帧数
    @param [out] metrics 吞吐、耗时、CPU、内存、带宽及三项延迟；缺失为-1
    @return 输出完整、数值有效且交付数正确
    */
    static bool parse(const std::string& output, int kind, uint64_t expected, double* metrics);

    /** 读取缓冲借用本上下文固定存储，不在回调中分配读缓冲
    @param [in] handle 当前管道
    @param [in] suggested libuv建议值，本实现不使用
    @param [out] buffer 固定读块
    */
    static void allocate(uv_handle_t* handle, size_t suggested, uv_buf_t* buffer);

    /** 收集stdout，EOF后关闭管道；任何异常标记该样本失败
    @param [in] stream 当前管道
    @param [in] count 字节数或libuv错误
    @param [in] buffer 本次借用读块
    */
    static void read(uv_stream_t* stream, ssize_t count, const uv_buf_t* buffer);

    /** 记录真实退出状态并关闭进程句柄，不丢弃尚未读完的stdout
    @param [in] process 本次创建的子进程
    @param [in] code 退出码
    @param [in] signal 终止信号
    */
    static void exited(uv_process_t* process, int64_t code, int signal);

    /** 只终止本上下文创建的超时子进程，不能把超时算作通过
    @param [in] timer 本批超时计时器
    */
    static void timeout(uv_timer_t* timer);

    /** 进程退出和stdout收敛都完成后停止超时计时器
    */
    void finish();

private:
    // 本次子进程专用循环，不使用生产对象的loop
    uv_loop_t m_loop;
    // 精确跟踪本次创建的进程
    uv_process_t m_process;
    // 子进程标准输出管道
    uv_pipe_t m_pipe;
    // 120秒截止时限
    uv_timer_t m_timer;
    // 管道固定读缓冲
    char m_buffer[4096];
    // 有上限的原始输出
    std::string m_output;
    // 已经观察进程终态
    bool m_exited;
    // 已经观察stdout结束
    bool m_eof;
    // 超时、信号、退出码或管道失败
    bool m_failed;
};