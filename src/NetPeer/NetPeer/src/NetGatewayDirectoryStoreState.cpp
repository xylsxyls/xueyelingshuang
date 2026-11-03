#include "NetGatewayDirectoryStoreState.h"
#include "NetPeerMacro.h"
#include "NetHostSocket.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <chrono>
#include <cstring>

NetGatewayDirectoryStoreState::NetGatewayDirectoryStoreState() :
m_entropy(nullptr),
m_context(nullptr),
m_phase(NET_CACHE_CLOSED),
m_job(0),
m_file(0),
m_length(0),
m_lastWrite(0),
m_operation(0),
m_result(NI_OK),
m_loadedReady(false)
{

}