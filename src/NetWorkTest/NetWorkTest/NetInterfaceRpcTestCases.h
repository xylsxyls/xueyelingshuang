#pragma once

// RPC编解码、关联与取消测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceRpcTestCases
{
public:
    /** 独立向量验证本地服务凭据、来源上下文与最大消息边界
    */
    static void serviceWire();

    /** 固定独立字节向量、完整64位父号及二进制业务体
    */
    static void wireVectors();

    /** 错误帧、容量、状态及真实C++主类入口边界
    */
    static void rejection();
};