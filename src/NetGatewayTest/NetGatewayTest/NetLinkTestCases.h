#pragma once

// 管理链路认证与有界收发测试集合，逐用例独立建立预期并检查实际结果
class NetLinkTestCases
{
public:
    /** 通过真实TLS测试畸形管理声明、挑战重放、伪造接受和首次计时
    @param [in] argc 参数数量
    @param [in] argv 命令及隔离identity.dat路径
    @return 0通过，非0为用例失败
    */
    static int authenticationAudit(int argc, char** argv);

    /** 使用隔离节点身份验证管理输出的容量、FIFO、失败回滚和缓冲复用
    @param [in] argc 参数数量
    @param [in] argv 命令及隔离identity.dat路径
    @return 0通过，非0为用例失败
    */
    static int outputQueueAudit(int argc, char** argv);

    /** 使用联调宿主生成的隔离身份验证管理输入的EOF排空边界
    @param [in] argc 参数数量，必须为2
    @param [in] argv 命令和隔离identity.dat路径
    @return 0通过，非0为用例失败
    */
    static int inputDrainAudit(int argc, char** argv);

#ifndef __EMSCRIPTEN__
    /** 白盒验证IPC管理层的原始END回调、背压和唯一关闭，不启动进程接收器
    @param [in] argc 参数数量，必须为2
    @param [in] argv 命令和隔离identity.dat路径
    @return 0通过，非0为用例失败
    */
    static int senderDrainAudit(int argc, char** argv);
#endif

    /** 原生/WasmC++协议入口、未认证业务拒绝、固定期限/退时和熵源错误
    */
    static void portableLink();

    /** 原生/Wasm同源额度：窗口耗尽拒绝无副作用、实际回传恢复、伪造重复额度持续拒绝
    */
    static void creditWindow();

    /** 协商窗口边界、旧端默认值及Wasm同源入口
    */
    static void negotiatedCredit();
};