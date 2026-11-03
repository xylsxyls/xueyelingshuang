#include "NetSenderGatewayState.h"
#include "NetSenderMacro.h"
#include "NetSenderPlatform.h"
#include "NetSenderLocalHub.h"
#include "NetSenderDesktopBackend.h"
#include "NetSenderGatewayLaunch.h"
#include <cstring>
#include "CSystem/CSystemAPI.h"

NetSenderGatewayState::NetSenderGatewayState() :
m_registered(false),
m_receiverLease(false),
m_dispatch(&m_session),
m_backendPid(0),
m_serviceProduct(0),
m_lastError(NI_INVALID_STATE)
{

}

NetInterfaceTlsOptions NetSenderGatewayState::clientTls(const NetInterfaceTlsOptions& options)
{
    NetInterfaceTlsOptions configured = options;
    if (configured.entropy == nullptr)
    {
        configured.entropy = CSystem::secureRandomBytesCallback;
        configured.entropyContext = nullptr;
    }
    return configured;
}