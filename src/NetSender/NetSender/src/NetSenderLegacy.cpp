#include "NetSenderLegacy.h"
#include "NetSenderMacro.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetSender.h"
#include "CSystem/CSystemAPI.h"
#include "NetSenderLegacyState.h"
#include <cstring>
#include <limits>

#define NET_CLIENT_VERSION "1.1"
#define NET_SERVER_VERSION "1.1"

static const uint32_t kLegacyPayloadLimit = 1048512;

/** 生成旧进程封包，先验证长度再分配/读取，不用未对齐的整数指针。
@param [in] first 第一项原生int32
@param [in] second 可选第二项原生int32
@param [in] pair 是否包含第二项
@param [in] buffer length非零时不可空，只借用到返回
@param [in] length 0至有界旧载荷上限
@return 有界封包，非法输入为空
*/
static std::string LegacyFrame(int32_t first, int32_t second, bool pair, const char* buffer, int32_t length)
{
    if (length < 0 || static_cast<uint32_t>(length) > kLegacyPayloadLimit || (length != 0 && buffer == nullptr))
    {
        return std::string();
    }
    const uint32_t header = pair ? 8U : 4U;
    std::string result(header + static_cast<uint32_t>(length), '\0');
    std::memcpy(&result[0], &first, sizeof(first));
    if (pair)
    {
        std::memcpy(&result[4], &second, sizeof(second));
    }
    if (length != 0)
    {
        std::memcpy(&result[header], buffer, static_cast<uint32_t>(length));
    }
    return result;
}

#define NET_CLIENT_VERSION "1.1"
#define NET_SERVER_VERSION "1.1"

//发送类型由发送方决定
// 旧接口仍保留给尚未迁移的工程；它不属于安全网关接入。

NetSenderLegacy::NetSenderLegacy() :
m_legacy(new NetSenderLegacyState())
{

}

NetSenderLegacy::~NetSenderLegacy()
{
    delete m_legacy;
}

NetSenderLegacy& NetSenderLegacy::instance()
{
    static NetSenderLegacy s_netSender;
    return s_netSender;
}

void NetSenderLegacy::sendServer(int32_t serverId, const char* buffer, int32_t length)
{
    const std::string packet = LegacyFrame(serverId, m_legacy->m_currentProcessPid, true, buffer, length);
    if (!packet.empty())
    {
        ProcessWork::instance().send(m_legacy->m_netClientManagerName, packet.data(), static_cast<int32_t>(packet.size()), MessageType::MESSAGE);
    }
}

void NetSenderLegacy::sendServer(int32_t serverId, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return;
    }
    sendServer(serverId, message.c_str(), static_cast<int32_t>(message.size()));
}

void NetSenderLegacy::sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
    const std::string packet = LegacyFrame(connectId, clientPid, true, buffer, length);
    if (!packet.empty())
    {
        ProcessWork::instance().send(m_legacy->m_netServerManagerName, packet.data(), static_cast<int32_t>(packet.size()), MessageType::SERVER_MESSAGE);
    }
}

void NetSenderLegacy::sendClient(int32_t connectId, int32_t clientPid, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return;
    }
    sendClient(connectId, clientPid, message.c_str(), static_cast<int32_t>(message.size()));
}

void NetSenderLegacy::postServer(int32_t serverId, const char* buffer, int32_t length)
{
    const std::string packet = LegacyFrame(serverId, m_legacy->m_currentProcessPid, true, buffer, length);
    if (!packet.empty())
    {
        ProcessWork::instance().post(m_legacy->m_netClientManagerName, packet.data(), static_cast<int32_t>(packet.size()), MessageType::MESSAGE);
    }
}

void NetSenderLegacy::postServer(int32_t serverId, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return;
    }
    postServer(serverId, message.c_str(), static_cast<int32_t>(message.size()));
}

void NetSenderLegacy::postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
    const std::string packet = LegacyFrame(connectId, clientPid, true, buffer, length);
    if (!packet.empty())
    {
        ProcessWork::instance().post(m_legacy->m_netServerManagerName, packet.data(), static_cast<int32_t>(packet.size()), MessageType::SERVER_MESSAGE);
    }
}

void NetSenderLegacy::postClient(int32_t connectId, int32_t clientPid, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return;
    }
    postClient(connectId, clientPid, message.c_str(), static_cast<int32_t>(message.size()));
}

std::string NetSenderLegacy::netServerMessage(int32_t clientPid, const char* buffer, int32_t length)
{
    return LegacyFrame(clientPid, 0, false, buffer, length);
}

std::string NetSenderLegacy::netServerMessage(int32_t clientPid, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return std::string();
    }
    return netServerMessage(clientPid, message.c_str(), static_cast<int32_t>(message.size()));
}

std::string NetSenderLegacy::netClientMessage(int32_t serverId, const char* buffer, int32_t length)
{
    return LegacyFrame(serverId, m_legacy->m_currentProcessPid, true, buffer, length);
}

std::string NetSenderLegacy::netClientMessage(int32_t serverId, const std::string& message)
{
    if (message.size() > kLegacyPayloadLimit)
    {
        return std::string();
    }
    return netClientMessage(serverId, message.c_str(), static_cast<int32_t>(message.size()));
}

void NetSenderLegacy::initClientReceive(ClientReceiveCallback* callback)
{
    ProcessWork::instance().addProcessReceiveCallback(callback);
}

void NetSenderLegacy::initServerReceive(ServerReceiveCallback* callback)
{
    ProcessWork::instance().addProcessReceiveCallback(callback);
}

void NetSenderLegacy::initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo)
{
    if (initInfo.size() > kLegacyPayloadLimit)
    {
        return;
    }
    //客户端将自己的进程号名字及要发的服务进程名和初始化附带信息发送给服务端
    ProtoMessage message;
    message[CLIENT_PID] = m_legacy->m_currentProcessPid;
    message[CLIENT_NAME] = m_legacy->m_currentExeFullName;
    message[SERVER_ID] = serverId;
    message[SERVER_NAME] = serverName;
    message[CLIENT_INIT_INFO] = initInfo;
    std::string strMessage = message.toString();
    if (strMessage.size() > kLegacyPayloadLimit)
    {
        return;
    }
    ProcessWork::instance().send(m_legacy->m_netClientManagerName,
        strMessage.c_str(),
        static_cast<int32_t>(strMessage.length()),
        MessageType::CLIENT_INIT);
}

void NetSenderLegacy::sendClientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo)
{
    if (responseInfo.size() > kLegacyPayloadLimit)
    {
        return;
    }
    ProtoMessage message;
    message[CONNECT_ID] = connectId;
    message[CLIENT_PID] = clientPid;
    message[CLIENT_INIT_RESPONSE_INFO] = responseInfo;
    std::string strMessage = message.toString();
    if (strMessage.size() > kLegacyPayloadLimit)
    {
        return;
    }
    ProcessWork::instance().send(m_legacy->m_netServerManagerName,
        strMessage.c_str(),
        static_cast<int32_t>(strMessage.length()),
        MessageType::CLIENT_INIT_RESPONSE);
}

void NetSenderLegacy::initServer(const std::string& initInfo)
{
    if (initInfo.size() > kLegacyPayloadLimit)
    {
        return;
    }
    ProcessWork::instance().send(m_legacy->m_netServerManagerName,
        initInfo.c_str(),
        static_cast<int32_t>(initInfo.length()),
        MessageType::SERVER_INIT);
}

// 旧二进制继续转发到真实兼容后端，公共业务声明不再暴露这些方法。