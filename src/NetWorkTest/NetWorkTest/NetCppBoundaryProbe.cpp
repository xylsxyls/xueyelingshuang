#include "NetSender/NetSenderAPI.h"
#include "NetPeer/NetPeerAPI.h"
#include "NetWork/NetWorkAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetCppBoundaryProbe.h"
#include "NetWorkTestCases.h"
#include <stdexcept>

// 与C函数同名的类型用于编译期回归；若默认入口泄漏C声明，此翻译单元会编译失败
typedef int NetPeerNetSender;
typedef int NetWorkNetSender;
typedef int netSenderClientCreate;
typedef int netPeerConfigureHost;
typedef int netPeerRandomBytes;
typedef int netWorkClientCreate;
typedef int netWorkConfigureHost;
typedef int libuvHostCreate;

void NetCppBoundaryProbe::cppBoundary()
{
    netTestCheckpoint();
    NetHostFunctions invalid = {};
    uint8_t sentinel = 0xa5;
    void* instance = reinterpret_cast<void*>(1);
    if (NetPeer::pointerBytes() != sizeof(void*) || NetWork::pointerBytes() != sizeof(void*) ||
        NetPeer::configureHost(invalid) != NI_INVALID_ARGUMENT || NetWork::configureHost(invalid) != NI_INVALID_ARGUMENT ||
        NetPeer::randomBytes(nullptr, 1) != NI_INVALID_ARGUMENT || NetWork::randomBytes(nullptr, 1) != NI_INVALID_ARGUMENT ||
        NetPeer::randomBytes(&sentinel, 8193) != NI_INVALID_ARGUMENT || sentinel != 0xa5 ||
        NetWork::randomBytes(&sentinel, 8193) != NI_INVALID_ARGUMENT || sentinel != 0xa5 ||
        LibuvTcp::hostCreate(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &instance) >= 0 || instance != nullptr ||
        LibuvTcp::hostDestroy(nullptr) != 0 || LibuvTcp::hostPoll(nullptr) >= 0)
    {
        throw std::runtime_error("C++ boundary or host contract");
    }
}