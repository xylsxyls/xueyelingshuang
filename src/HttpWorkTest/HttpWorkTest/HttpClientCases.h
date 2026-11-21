#pragma once
#include "HttpRequest/HttpRequestAPI.h"
#include <atomic>
#include <memory>
#include <vector>
#ifndef __EMSCRIPTEN__
#include <condition_variable>
#include <mutex>
#endif

/** 异步测试的状态，回调持有共享引用直至最后一个检查完成
*/
class HttpClientCases : public std::enable_shared_from_this<HttpClientCases>
{
public:
    /** 创建未完成的请求测试
    */
    HttpClientCases();

    /** 发布最后一个检查的结果并通知等待者
    @param [in] success 是否通过
    @param [in] index 检查序号
    */
    void finish(bool success, int index);

    /** 对实际源站发送下一项功能用例
    @param [in] index 首个用例从零开始
    */
    void run(int index);

    /** 在源站完成计数复位后启动并发订阅，比较两个回调与源站观测。 */
    void runCoalescing();

    /** 连续发送十次预热及九十次计时 GET
    @param [in] index 当前请求序号，每次在前一次完成后推进
    */
    void benchmark(size_t index);

public:
    // 本次测试实际可达的源站
    std::string m_origin;
    // 全部用例的基础证书、超时及响应上限
    HttpClientOptions m_options;
    // 运行不受信证书或错误主机名用例
    bool m_reject;
    // 对指定公网入口执行系统信任检查
    bool m_public;
    // 使用同源长连接端点的第二阶段原生基准
    bool m_keepAlive;
    // -1 等待，0 通过，1 失败
    int m_result;
    // 在途合并用例完成的回调总数
    std::atomic<int> m_finished;
    // 在途合并用例的各回调是否全部成功
    std::atomic<bool> m_correct;
    // 完成回调实测的端到端微秒数
    std::vector<double> m_samples;
    // 已选客户端场景，按稳定编号 3 至 12 的顺序登记。
    std::vector<bool> m_selected;
#ifndef __EMSCRIPTEN__
    // 在所有者线程及完成回调之间保护最终状态
    std::mutex m_mutex;
    // 唤醒等待所有用例完成的所有者线程
    std::condition_variable m_done;
#endif
};

/** 运行 HttpRequest 客户端功能、证书或性能用例
@param [in] argc 参数个数，至少三个
@param [in] argv 测试模式和源站绝对地址，原生端可另传本次 CA 文件
@return 原生返回最终退出状态，Wasm 异步状态通过 httpWorkTestResult 读取
*/
int HttpClientCasesMain(int argc, char* argv[]);

#ifndef __EMSCRIPTEN__
/** 验证原生连接复用、选项重置及信任切换
@param [in] argc 程序、模式、两个源站及可选私有 CA 文件的参数个数
@param [in] argv 原生命令行参数
@return 所有检查通过时为零
*/
int HttpClientReuseCases(int argc, char* argv[]);
#endif