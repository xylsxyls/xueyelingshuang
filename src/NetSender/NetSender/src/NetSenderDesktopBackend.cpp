#include "NetSenderMacro.h"
#include "NetSenderDesktopBackend.h"
#include "NetSenderGatewayState.h"
#include "NetSenderLocalHub.h"

int32_t NetSenderDesktopBackend::configureGateway(NetSenderGatewayState* client, int32_t pid)
{
    if (pid < 0) { return NI_INVALID_ARGUMENT; }
    if (client == nullptr || !client->m_dispatch.quiescent()) { return NI_INVALID_STATE; }
    client->m_backendPid = pid;
    return NI_OK;
}

bool NetSenderDesktopBackend::quiescent(const NetSenderGatewayState* client)
{
    return client == nullptr || client->m_dispatch.quiescent();
}

int32_t NetSenderDesktopBackend::prepareExternalReceiver()
{
    return NetSenderLocalHub::instance().prepareExternal();
}

int32_t NetSenderDesktopBackend::externalReceiverStopped()
{
    return NetSenderLocalHub::instance().externalStopped();
}