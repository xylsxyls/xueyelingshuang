#pragma once

// 能力握手、参数和身份声明测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceHandshakeTestCases
{
public:
    /** 手写64字节向量、能力收敛和输出背压
    */
    static void negotiation();

    /** 坏版本/参数/保留位、握手前业务及握手重入均拒绝
    */
    static void rejection();

    /** 固定截止时间、时钟回退和客户端随机数绑定
    */
    static void deadlines();

    /** 真实平面C++主类入口，空参数清输出及同源Wasm调用
    */
    static void cBoundary();

    /** 同源真实TLS加密承载应用握手，再进行完整64位编号业务往返
    */
    static void tlsPipeline();
};