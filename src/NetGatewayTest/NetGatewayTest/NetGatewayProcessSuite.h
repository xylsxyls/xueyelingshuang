#pragma once
#include "NetTestProcess.h"
#include <memory>
#include <string>
#include <vector>

// 真实进程链路的C++夹具；保存每一个自建进程，不借用或终止现存业务进程
class NetGatewayProcessSuite
{
public:
    /** 初始化本批次目录与候选程序位置，目录须由入口独占创建
    @param [in] binaries 配套候选EXE/DLL目录
    @param [in] output 空的本次测试目录
    */
    NetGatewayProcessSuite(const std::string& binaries, const std::string& output);

    /** 先通知所有自有常驻进程停止，再回收剩余自有句柄
    */
    ~NetGatewayProcessSuite();

    /** 调用指定真实进程场景，所有失败均返回非零
    @param [in] argc 完整命令行项数
    @param [in] argv --process-fixture选择parser/replacement/singleton/background/luma，另含--bin和--output
    @return 0全部通过，1用例失败，2参数或平台不支持，3报告写入失败
    */
    static int run(int argc, char** argv);

private:
    /** 禁止复制运行进程的所有权
    @param [in] other 原对象
    */
    NetGatewayProcessSuite(const NetGatewayProcessSuite& other);

    /** 禁止赋值运行进程的所有权
    @param [in] other 原对象
    @return 不提供实现
    */
    NetGatewayProcessSuite& operator=(const NetGatewayProcessSuite& other);

    /** 验证生产单例租约未被其他进程占用，不发送替换或停止消息
    @return 能证明客户端、服务端和守护租约均空闲返回true
    */
    static bool leasesFree();

    /** 启动一个候选进程并登记终止所有权
    @param [in] name 本用例日志和停止文件的唯一名称
    @param [in] filename 候选目录内EXE名称
    @param [in] arguments 除驻留参数外的原始参数
    @param [in] resident 是否自动添加本用例停止文件和90秒兜底期限
    @param [in] normalStartup Windows正常启动，不由夹具隐藏被测GUI
    @return 本夹具借用的子进程；失败抛出测试异常
    */
    NetTestProcess& launch(const std::string& name, const std::string& filename,
        const std::vector<std::string>& arguments, bool resident, bool normalStartup = false);

    /** 从本次部署目录启动程序并登记作业所有权
    @param [in] name 本次唯一日志名称
    @param [in] executable 完整候选路径
    @param [in] arguments 原始参数
    @param [in] normalStartup Windows不强制隐藏程序
    @return 夹具拥有的子进程；失败抛出测试异常
    */
    NetTestProcess& launchPath(const std::string& name, const std::string& executable,
        const std::vector<std::string>& arguments, bool normalStartup = false);

    /** 等待完整日志行，同时检查本次进程身份和生存状态
    @param [in,out] process 本夹具拥有的目标进程
    @param [in] prefix 日志行前缀，必须从行首开始
    @param [in] timeoutMs 总期限，包含进程提前退出判断
    @return 匹配的完整行；失败抛出测试异常
    */
    std::string waitLine(NetTestProcess& process, const std::string& prefix, uint32_t timeoutMs);

    /** 从完整日志行读取一个无溢出十进制字段，不接受只匹配数字前缀
    @param [in] line 完整日志行
    @param [in] key 包含等号的字段名
    @return 正数；不存在、零、重复或非十进制时抛出测试异常
    */
    static uint32_t number(const std::string& line, const std::string& key);

    /** 要求该进程按期正常退出，非零、超时和崩溃均为失败
    @param [in,out] process 本夹具拥有的目标进程
    @param [in] timeoutMs 最长等待毫秒
    */
    static void exited(NetTestProcess& process, uint32_t timeoutMs);

    /** 向已登记的驻留进程发停止信号，不能创建未经本夹具登记的停止文件
    @param [in] name launch时的本批次名称
    */
    void stop(const std::string& name);

    /** 启动真实服务端，端口0表示由系统分配
    @param [in] name 本批次唯一名称
    @param [in,out] port 输入监听端口；成功后写实际端口
    @return 借用的服务端进程
    */
    NetTestProcess& server(const std::string& name, uint32_t& port);

    /** 启动真实业务服务，并核对就绪PID确为自有进程
    @param [in] name 本批次唯一名称
    @param [in] gateway 服务端网关的实际PID
    @return 借用的业务服务进程
    */
    NetTestProcess& service(const std::string& name, uint32_t gateway);

    /** 启动真实桌面网关，使用本批次独立缓存和停止文件
    @param [in] name 本批次唯一名称
    @param [in] port 自有服务端监听端口
    @return 借用的客户端网关进程
    */
    NetTestProcess& client(const std::string& name, uint32_t port);

    /** 客户端替换及服务端替换后验证实际业务回调、新代次和完整请求回应
    */
    void replacement();

    /** 真实进程的并发复用、默认启动语义、替换、崩溃释放和守护恢复；不复制生产实现
    */
    void singleton();

    /** 验证GUI驻留、守护、双次崩溃业务恢复、会话结束、部署及可逆登录项
    */
    void background();

    /** 验证中文空格部署、同级目录优先和缺失文件；前提为客户端租约空闲
    @param [in] port 本测试服务端的监听端口
    */
    void deployments(uint32_t port);

    /** 验证安装、重复卸载登录项，退出前恢复原值及类型
    */
    void startup();

    /** 运行独立业务服务和业务测试程序的真实网关联调，不修改业务工程
    @param [in] serverExecutable 配套LumaPlayerServer候选映像
    @param [in] testExecutable 配套LumaPlayerServerTest候选映像
    */
    void luma(const std::string& serverExecutable, const std::string& testExecutable);

    /** 等待指定作业里的候选网关恢复，核对只有一个就绪实例及一个原守护
    @param [in] root 拥有守护及恢复后代的启动进程
    @param [in] previous 必须已被替换的旧PID
    @param [in] guard 必须保持不变的原守护PID
    @return 新就绪网关PID；期限内未恢复或多实例时抛出测试异常
    */
    uint32_t recovered(NetTestProcess& root, uint32_t previous, uint32_t guard);

    /** 证明本次作业中的指定映像已经全部退出，包括守护启动的后代
    @param [in] root 本次启动的根进程
    @param [in] filename 候选目录内映像名
    @param [in] timeoutMs 检查期限
    */
    void descendantsStopped(NetTestProcess& root, const std::string& filename, uint32_t timeoutMs);

private:
    // 候选程序所在目录，整个夹具期间保持不变
    std::string m_binaries;
    // 本批次独占目录，不复用旧日志或节点身份
    std::string m_output;
    // 自建直接子进程的唯一所有权
    std::vector<std::unique_ptr<NetTestProcess>> m_children;
    // 所有已登记常驻进程的停止文件名
    std::vector<std::string> m_residents;
};