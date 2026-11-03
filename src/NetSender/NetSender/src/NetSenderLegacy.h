#pragma once
#include "NetSenderMacro.h"
#include "ClientReceiveCallback.h"
#include "ServerReceiveCallback.h"
#include <string>

class NetSenderLegacyState;

// 仅旧桌面注册及旧消息兼容，不属于安全NetSender业务门面
class NetSenderAPI NetSenderLegacy
{
private:
    /** 创建本模块兼容状态，不启动接收
    */
    NetSenderLegacy();
public:
    /** 释放本模块状态，旧ProcessWork观察者须已停止
    */
    ~NetSenderLegacy();

    /** 取得旧桌面协议兼容对象
    @return 本模块进程生命期实例
    */
    static NetSenderLegacy& instance();
public:
    /** 登记旧桌面客户端观察者，必须在ProcessWork.initReceive之前
    @param [in] callback 借用至ProcessWork停止，不可在观察者回调销毁
    */
    void initClientReceive(ClientReceiveCallback* callback);

    /** 登记旧桌面服务观察者
    @param [in] callback 借用至ProcessWork停止
    */
    void initServerReceive(ServerReceiveCallback* callback);

    /** 旧桌面客户端注册；该兼容协议不作为新的安全业务入口
    @param [in] serverId 旧目标号
    @param [in] serverName Windows不带后缀、Linux全名
    @param [in] initInfo 旧注册载荷，借用至返回
    */
    void initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo);

    /** 旧桌面注册响应
    @param [in] connectId 旧连接号
    @param [in] clientPid 原客户端PID
    @param [in] responseInfo 借用响应
    */
    void sendClientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo);

    /** 旧桌面服务注册
    @param [in] initInfo 旧注册内容
    */
    void initServer(const std::string& initInfo);

    /** 旧桌面兼容发送，不替代新TLS请求接口
    @param [in] serverId 旧目标号
    @param [in] message 调用时借用的旧载荷
    */
    void sendServer(int32_t serverId, const std::string& message);

    /** 旧桌面兼容发送，不替代新TLS请求接口
    @param [in] serverId 旧目标号
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负有效字节数
    */
    void sendServer(int32_t serverId, const char* buffer, int32_t length);

    /** 旧桌面兼容发送，不替代新TLS请求接口
    @param [in] connectId 旧连接号
    @param [in] clientPid 原客户端PID
    @param [in] message 调用时借用的旧载荷
    */
    void sendClient(int32_t connectId, int32_t clientPid, const std::string& message);

    /** 旧桌面兼容发送，不替代新TLS请求接口
    @param [in] connectId 旧连接号
    @param [in] clientPid 原客户端PID
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负有效字节数
    */
    void sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

    /** 旧桌面异步投递，宿主须先初始化ProcessWork postThread
    @param [in] serverId 旧目标号
    @param [in] message 调用时借用的旧载荷
    */
    void postServer(int32_t serverId, const std::string& message);

    /** 旧桌面异步投递，宿主须先初始化ProcessWork postThread
    @param [in] serverId 旧目标号
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负有效字节数
    */
    void postServer(int32_t serverId, const char* buffer, int32_t length);

    /** 旧桌面异步投递，宿主须先初始化ProcessWork postThread
    @param [in] connectId 旧连接号
    @param [in] clientPid 原客户端PID
    @param [in] message 调用时借用的旧载荷
    */
    void postClient(int32_t connectId, int32_t clientPid, const std::string& message);

    /** 旧桌面异步投递，宿主须先初始化ProcessWork postThread
    @param [in] connectId 旧连接号
    @param [in] clientPid 原客户端PID
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负有效字节数
    */
    void postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

    /** 旧桌面封包兼容入口
    @param [in] clientPid 旧路由标识，0表示对应Manager
    @param [in] message 借用旧载荷
    @return 兼容封包；不代表发送或认证成功
    */
    std::string netServerMessage(int32_t clientPid, const std::string& message);

    /** 旧桌面封包兼容入口
    @param [in] clientPid 旧路由标识，0表示对应Manager
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负字节数
    @return 兼容封包；不代表发送或认证成功
    */
    std::string netServerMessage(int32_t clientPid, const char* buffer, int32_t length);

    /** 旧桌面封包兼容入口
    @param [in] serverId 旧路由标识，0表示对应Manager
    @param [in] message 借用旧载荷
    @return 兼容封包；不代表发送或认证成功
    */
    std::string netClientMessage(int32_t serverId, const std::string& message);

    /** 旧桌面封包兼容入口
    @param [in] serverId 旧路由标识，0表示对应Manager
    @param [in] buffer length非零时不可空，借用至返回
    @param [in] length 非负字节数
    @return 兼容封包；不代表发送或认证成功
    */
    std::string netClientMessage(int32_t serverId, const char* buffer, int32_t length);

private:
    /** 禁止复制旧兼容状态
    @param [in] other 不可复制的活动对象
    */
    NetSenderLegacy(const NetSenderLegacy& other);

    /** 禁止赋值持有活动协议状态的对象
    @param [in] other 不可赋值的实例
    @return 不可调用
    */
    NetSenderLegacy& operator=(const NetSenderLegacy& other);
    // 仅本模块分配／释放
    NetSenderLegacyState* m_legacy;
};