#include "LibuvTcp/LibuvTcpAPI.h"
#include <LibuvTcp/LibuvTcpAPI.h>
#include "NetWorkConcurrencyTestCases.h"
#include "NetWorkTestAccess.h"
#include "NetSenderTestAccess.h"
#include "NetTestTlsCertificates.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetWorkTestCases.h"
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#endif

LibuvAcceptTestState::LibuvAcceptTestState() :
m_peer(0),
m_connected(0),
m_closed(0),
m_received(0),
m_valid(true)
{

}