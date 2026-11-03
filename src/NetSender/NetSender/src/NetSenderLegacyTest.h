#pragma once
#include "ClientReceiveCallback.h"
#include "ServerReceiveCallback.h"

// 仅旧协议的长度/类型观察器，不启动ProcessWork或网络。
class NetSenderLegacyClientProbe : public ClientReceiveCallback
{
public:
    /** 初始化计数。
    */
    NetSenderLegacyClientProbe();

    /** 直接投递受控输入。
    @param [in] type 旧类型
    @param [in] bytes 借用数据
    @param [in] size 有符号受控长度
    */
    void inject(MessageType type, const char* bytes, int32_t size);

    /** 记录有效初始化。
    @param [in] serverId 服务器号
    @param [in] bytes 借用正文
    @param [in] size 长度
    */
    virtual void clientInitResponse(int32_t serverId, const char* bytes, int32_t size);

    /** 记录有界业务回调。
    @param [in] serverId 服务器号
    @param [in] bytes 借用正文
    @param [in] size 长度
    */
    virtual void ServerMessage(int32_t serverId, const char* bytes, int32_t size);
    // 有效业务回调数。
    uint32_t m_calls;
};

// 兼容接入测试中的服务端回调，只记录实际ProcessWork投递
class NetSenderLegacyServerProbe : public ServerReceiveCallback
{
public:
    /** 初始化计数。
    */
    NetSenderLegacyServerProbe();

    /** 直接投递受控输入。
    @param [in] type 旧类型
    @param [in] bytes 借用数据
    @param [in] size 有符号受控长度
    */
    void inject(MessageType type, const char* bytes, int32_t size);

    /** 记录有效初始化。
    @param [in] connection 连接号
    @param [in] pid 旧客户端号
    @param [in] bytes 借用正文
    @param [in] size 长度
    */
    virtual void clientInit(int32_t connection, int32_t pid, const char* bytes, int32_t size);

    /** 记录有效初始化回应。
    @param [in] bytes 借用正文
    @param [in] size 长度
    */
    virtual void serverInitResponse(const char* bytes, int32_t size);

    /** 记录有界业务回调。
    @param [in] connection 连接号
    @param [in] pid 旧客户端号
    @param [in] bytes 借用正文
    @param [in] size 长度
    */
    virtual void ClientMessage(int32_t connection, int32_t pid, const char* bytes, int32_t size);
    // 有效业务回调数。
    uint32_t m_calls;
};