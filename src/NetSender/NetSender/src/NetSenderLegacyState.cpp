#include "NetSenderLegacyState.h"
#include "NetSenderMacro.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetSender.h"
#include "NetSenderLegacy.h"
#include "CSystem/CSystemAPI.h"
#include <cstring>
#include <limits>

#define NET_CLIENT_VERSION "1.1"
#define NET_SERVER_VERSION "1.1"

NetSenderLegacyState::NetSenderLegacyState() :
m_currentProcessPid(static_cast<int32_t>(CSystem::currentProcessPid())),
m_currentExeFullName(CSystem::GetCurrentExeFullName()),
m_netClientManagerName(std::string("NetClientManager") + NET_CLIENT_VERSION),
m_netServerManagerName(std::string("NetServerManager") + NET_SERVER_VERSION)
{

}