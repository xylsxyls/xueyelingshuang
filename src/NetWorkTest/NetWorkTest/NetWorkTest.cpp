#include "NetWorkTest.h"
#include "NetSenderTestClient.h"
#include "NetWorkTestRunner.h"
#include "NetWorkTestCases.h"
#ifndef __EMSCRIPTEN__
#include "NetWork/NetWorkAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "CSystem/CSystemAPI.h"
// 原生文件后端由NetWork正式宿主入口配置。
#endif
#include "NetIdentityTestCases.h"
#include "NetTunnelTestCases.h"
#include "NetLinkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterfaceProtoTestCases.h"
#include "NetInterfaceConnectionTestCases.h"
#include "NetInterfaceHandshakeTestCases.h"
#include "NetInterfaceRequestsTestCases.h"
#include "NetInterfaceRpcTestCases.h"
#ifndef __EMSCRIPTEN__
#include "NetInterfaceNativeTestCases.h"
#include "NetRelayTransportProbe.h"
#include "NetInterfaceTransportProbe.h"
#include "NetInterfaceIpcTestCases.h"
#include "NetGatewayTestCases.h"
#include "NetLocalTestCases.h"
#include "NetRawLocalTestCases.h"
#include "NetLinkLocalTestCases.h"
#include "NetGatewayClientTestProbe.h"
#include "NetEndpointRouterTestCases.h"
#include "NetWorkInProcessTestCases.h"
#include "NetCppBoundaryProbe.h"
#include "NetSenderFacadeConcurrency.h"
#include "NetSenderFacadeNetPeerCppTest.h"
#include "NetSenderFacadeNetPeerCTest.h"
#include "NetSenderFacadeNetSenderCppTest.h"
#include "NetSenderFacadeNetWorkCppTest.h"
#include "NetSenderFacadeNetWorkCTest.h"
#include "NetWorkConcurrencyTestCases.h"
#include "NetServiceTestCases.h"
#include "NetAccountGatewayProbe.h"
#include "NetFileGatewayProbe.h"
#include "NetSender/NetSenderAPI.h"
#endif
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

static volatile sig_atomic_t g_cancelRequested = 0;
static std::chrono::steady_clock::time_point g_deadline;

static const NetWorkTestEntry g_tests[] = {
    {110, "File snapshots preserve active stream after progress and resumed hashing", false, &NetWorkTestCases::fileFlowSnapshot},
    {63, "\xe6\x96\x87\xe4\xbb\xb6\xe5\x8d\x8f\xe8\xae\xae\xe6\x8c\x81\xe4\xb9\x85\xe7\xa1\xae\xe8\xae\xa4\xe5\x9f\xba\xe7\xa1\x80\xe9\x97\xad\xe7\x8e\xaf", false, &NetWorkTestCases::fileFlowBasic},
    {62, "\xe5\xa4\xa7\xe6\x96\x87\xe4\xbb\xb6\xe6\xb5\x81\xe5\xbc\x8f\xe6\x91\x98\xe8\xa6\x81\xe5\x9f\xba\xe7\xa1\x80\xe5\x90\x91\xe9\x87\x8f", false, &NetWorkTestCases::streamingHash},
    {118, "NIF2编码输入输出别名边界", false, &NetWorkTestCases::encodingAliases},
    {1, "\xe5\x9b\xba\xe5\xae\x9a\xe7\xba\xbf\xe6\xa0\xbc\xe5\xbc\x8f", false, &NetWorkTestCases::wireVector},
    {2, "\xe6\x89\x80\xe6\x9c\x89\xe5\x88\x87\xe5\x88\x86", false, &NetWorkTestCases::fragmentation},
    {3, "\xe7\xb2\x98\xe5\x8c\x85\xe4\xb8\x8e\xe8\x83\x8c\xe5\x8e\x8b", false, &NetWorkTestCases::backpressure},
    {4, "\xe9\x9d\x9e\xe6\xb3\x95\xe5\xa4\xb4", false, &NetWorkTestCases::invalidHeaders},
    {5, "\xe9\x95\xbf\xe5\xba\xa6\xe9\x99\x90\xe5\x88\xb6", false, &NetWorkTestCases::sizeLimits},
    {6, "\xe7\xa9\xba\xe8\xbe\x93\xe5\x85\xa5\xe4\xb8\x8e\xe6\x88\xaa\xe6\x96\xad", false, &NetWorkTestCases::endOfInput},
    {7, "\xe7\xbc\x96\xe7\xa0\x81\xe4\xb8\x8e\xe5\x8f\x82\xe6\x95\xb0", false, &NetWorkTestCases::arguments},
    {8, "C/C++ public entry", false, &NetWorkTestCases::cLanguage},
    {9, "\xe7\xa1\xae\xe5\xae\x9a\xe6\x80\xa7\xe7\xa2\x8e\xe7\x89\x87\xe5\x9b\x9e\xe5\xbd\x92", false, &NetWorkTestCases::deterministic},
    {10, "\x54\x4c\x53\xe6\x8f\xa1\xe6\x89\x8b\xe4\xb8\x8e\xe5\x85\xb3\xe9\x97\xad", false, &NetInterfaceTlsTestCases::handshakeAndClose},
    {11, "\x54\x4c\x53\xe8\xaf\x81\xe4\xb9\xa6\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceTlsTestCases::certificateRejection},
    {12, "\x54\x4c\x53\xe7\xaf\xa1\xe6\x94\xb9\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceTlsTestCases::tamperRejection},
    {13, "\x54\x4c\x53\xe9\x87\x8d\xe6\x94\xbe\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceTlsTestCases::replayRejection},
    {14, "\x54\x4c\x53\xe6\x96\xad\xe6\xb5\x81\xe4\xb8\x8e\xe6\x97\xb6\xe9\x99\x90", false, &NetInterfaceTlsTestCases::failures},
    {15, "TLS C++ factory and entropy", false, &NetInterfaceTlsTestCases::cApiAndEntropy},
    {16, "\x54\x4c\x53\xe6\x9c\x89\xe7\x95\x8c\xe4\xbc\xa0\xe8\xbe\x93", false, &NetInterfaceTlsTestCases::boundedTransfer},
    {17, "\x70\x72\x6f\x74\x6f\x62\x75\x66\x33\x2e\x33\x2e\x30\xe6\xa0\xbc\xe5\xbc\x8f\xe5\x85\xbc\xe5\xae\xb9", false, &NetInterfaceProtoTestCases::wireCompatibility},
    {18, "\x70\x72\x6f\x74\x6f\x62\x75\x66\xe9\x9d\x9e\xe6\xb3\x95\xe5\xad\x97\xe6\xae\xb5", false, &NetInterfaceProtoTestCases::invalidValues},
    {19, "\x70\x72\x6f\x74\x6f\x62\x75\x66\xe4\xb8\x8e\x56\x61\x72\x69\x61\x6e\x74\xe6\x89\x80\xe6\x9c\x89\xe6\x9d\x83", false, &NetInterfaceProtoTestCases::ownership},
    {20, "protobuf C++ entry boundaries", false, &NetInterfaceProtoTestCases::cBoundary},
    {21, "\x70\x72\x6f\x74\x6f\x62\x75\x66\xe5\xae\xb9\xe9\x87\x8f\xe4\xb8\x8e\xe6\xb7\xb1\xe5\xba\xa6", false, &NetInterfaceProtoTestCases::limits},
#ifndef __EMSCRIPTEN__
    {66, "Native asynchronous file transfer", false, &NetInterfaceNativeTestCases::asyncFileTransfer},
    {65, "\xe5\xae\x9e\xe9\x99\x85\xe6\x96\x87\xe4\xbb\xb6\xe6\x8c\x81\xe4\xb9\x85\xe8\xaf\xbb\xe5\x86\x99\xe4\xb8\x8e\xe5\x8f\x91\xe5\xb8\x83\xe5\x9f\xba\xe7\xa1\x80\xe9\xaa\x8c\xe8\xaf\x81", false, &NetInterfaceNativeTestCases::durableFile},
    {64, "\xe7\xbd\x91\xe5\x85\xb3\xe6\x96\x87\xe4\xbb\xb6\xe6\xb5\x81\xe6\x98\xa0\xe5\xb0\x84\xe4\xb8\x8e\xe6\x92\xa4\xe9\x94\x80\xe5\x9f\xba\xe7\xa1\x80\xe9\x97\xad\xe7\x8e\xaf", false, &NetGatewayTestCases::fileRouting},
    {60, "\xe5\x8e\x9f\xe7\x94\x9f\xe5\xae\xa2\xe6\x88\xb7\xe7\xab\xaf\xe4\xbc\x9a\xe8\xaf\x9d\xe5\x9f\xba\xe7\xa1\x80\xe9\x97\xad\xe7\x8e\xaf", false, &NetGatewayTestCases::clientSession},
    {50, "\xe7\x9c\x9f\xe5\xae\x9e\xe7\xbd\x91\xe5\x85\xb3\xe4\xb8\xa4\xe7\xba\xa7\xe5\xbc\x82\xe6\xad\xa5\xe8\xb7\xaf\xe7\x94\xb1", false, &NetGatewayTestCases::nestedRoute},
    {51, "\xe7\xbd\x91\xe5\x85\xb3\xe5\x8d\x8f\xe5\x95\x86\xe4\xb9\xb1\xe5\xba\x8f\xe5\x8f\x8a\xe6\x9c\x89\xe7\x95\x8c\xe8\xbd\xac\xe5\x8f\x91", false, &NetGatewayTestCases::budgetsAndHandshakeOrder},
    {52, "\xe7\xbd\x91\xe5\x85\xb3\xe6\x9c\x8d\xe5\x8a\xa1\xe4\xbb\xa3\xe6\xac\xa1\xe5\x8f\x8a\xe9\x94\x99\xe8\xaf\xaf\xe6\x9d\xa5\xe6\xba\x90", false, &NetGatewayTestCases::serviceGeneration},
    {53, "\xe6\x9c\xac\xe5\x9c\xb0\x54\x4c\x53\xe7\x89\x87\xe5\x8f\x8a\xe7\xa1\xae\xe8\xae\xa4\xe7\xaa\x97\xe5\x8f\xa3", false, &NetInterfaceIpcTestCases::localPackets},
    {22, "\xe7\x9c\x9f\xe5\xae\x9e\x54\x43\x50\xe6\x9c\x89\xe7\x95\x8c\xe6\x94\xb6\xe5\x8f\x91", false, &NetInterfaceNativeTestCases::boundedTcp},
    {23, "\x54\x43\x50\xe5\xae\xb9\xe9\x87\x8f\xe5\x8f\x8a\xe7\x94\x9f\xe5\x91\xbd\xe5\x91\xa8\xe6\x9c\x9f", false, &NetInterfaceNativeTestCases::tcpLifecycle},
    {27, "\x4e\x65\x74\x57\x6f\x72\x6b\xe7\x9c\x9f\xe5\xae\x9e\xe7\xbb\x84\xe5\x90\x88\xe4\xbc\xa0\xe8\xbe\x93", false, &NetInterfaceTransportTestCases::nativeTransport},
    {28, "\x4e\x65\x74\x57\x6f\x72\x6b\xe8\xaf\x81\xe4\xb9\xa6\xe8\xb6\x85\xe6\x97\xb6\xe6\x96\xad\xe6\xb5\x81", false, &NetInterfaceTransportTestCases::transportFailures},
    {29, "\x4e\x65\x74\x57\x6f\x72\x6b\xe5\x9b\x9e\xe8\xb0\x83\xe5\x8f\x8a\xe5\x9b\x9e\xe6\x94\xb6", false, &NetInterfaceTransportTestCases::transportLifetime},
    {30, "\xe5\x85\xb1\xe4\xba\xab\xe5\x86\x85\xe5\xad\x98\xe5\xae\xb9\xe9\x87\x8f\xe5\x8f\x8a\xe6\x98\xa0\xe5\xb0\x84\xe5\x9b\x9e\xe6\x94\xb6", false, &NetInterfaceNativeTestCases::sharedMemoryCapacity},
    {31, "\xe8\xbf\x9b\xe7\xa8\x8b\xe9\x80\x9a\xe7\x9f\xa5\xe6\x9c\x89\xe7\x95\x8c\xe9\x87\x8d\xe8\xaf\x95", false, &NetInterfaceIpcTestCases::notifications},
    {32, "\xe6\x8e\xa5\xe6\x94\xb6\xe5\x81\x9c\xe6\xbb\x9e\xe5\x8f\x8a\xe5\xae\xb9\xe9\x87\x8f\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceIpcTestCases::stalledReceiver},
    {33, "\xe6\xb6\x88\xe6\x81\xaf\xe5\x8f\x91\xe5\xb8\x83\xe5\x8f\x8a\xe9\x80\x9a\xe7\x9f\xa5\xe9\x87\x8d\xe8\xaf\x95", false, &NetInterfaceIpcTestCases::publication},
#ifdef _WIN32
    {58, "Two service processes complete gateway RPC", false, &NetServiceTestCases::nestedProcesses},
    {59, "Wrong credentials and duplicate service", false, &NetServiceTestCases::rejectedServices},
    {74, "ProcessWork opaque records and window backpressure", false, &NetRawLocalTestCases::transfer},
    {116, "IPC结束回调晚于对端退出", false, &NetRawLocalTestCases::delayedEnd},
    {117, "IPC缺失结束包有界收敛", false, &NetRawLocalTestCases::missingEnd},
    {54, "ProcessWork TLS transfer and backpressure", false, &NetLocalTestCases::transfer},
    {55, "ProcessWork TLS certificate rejection", false, &NetLocalTestCases::certificateRejection},
    {56, "ProcessWork TLS peer exit", false, &NetLocalTestCases::peerExit},
    {34, "\x50\x72\x6f\x63\x65\x73\x73\x57\x6f\x72\x6b\xe7\x9c\x9f\xe5\xae\x9e\xe8\xb7\xa8\xe8\xbf\x9b\xe7\xa8\x8b\xe6\x8e\xa5\xe6\x94\xb6", false, &NetInterfaceIpcTestCases::childTransfer},
#endif
#ifdef __APPLE__
    {35, "\x6d\x61\x63\x4f\x53\xe5\x85\xb1\xe4\xba\xab\xe5\x86\x85\xe5\xad\x98\xe5\x8f\x8a\xe5\x8e\x9f\xe5\xad\x90\xe6\x93\x8d\xe4\xbd\x9c", false, &NetInterfaceIpcTestCases::appleResources},
#endif
#endif
    {24, "\x54\x4c\x53\xe5\xae\x8c\xe6\x95\xb4\xe5\xb8\xa7\xe8\xbf\x9e\xe6\x8e\xa5", false, &NetInterfaceConnectionTestCases::framedTransfer},
    {25, "\xe6\x8e\xa7\xe5\x88\xb6\xe6\xb6\x88\xe6\x81\xaf\xe7\x8b\xac\xe7\xab\x8b\xe5\xae\xb9\xe9\x87\x8f", false, &NetInterfaceConnectionTestCases::controlPriority},
    {26, "\xe5\x8a\xa0\xe5\xaf\x86\xe9\x9d\x9e\xe6\xb3\x95\xe5\xb8\xa7\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceConnectionTestCases::invalidPeerFrame},
    {36, "\xe5\xba\x94\xe7\x94\xa8\xe6\x8f\xa1\xe6\x89\x8b\xe5\x9b\xba\xe5\xae\x9a\xe5\x90\x91\xe9\x87\x8f", false, &NetInterfaceHandshakeTestCases::negotiation},
    {37, "\xe5\xba\x94\xe7\x94\xa8\xe6\x8f\xa1\xe6\x89\x8b\xe6\x8b\x92\xe7\xbb\x9d\xe5\xbc\x82\xe5\xb8\xb8", false, &NetInterfaceHandshakeTestCases::rejection},
    {38, "\xe5\xba\x94\xe7\x94\xa8\xe6\x8f\xa1\xe6\x89\x8b\xe6\x97\xb6\xe9\x99\x90\xe4\xb8\x8e\xe9\x9a\x8f\xe6\x9c\xba\xe6\x95\xb0", false, &NetInterfaceHandshakeTestCases::deadlines},
    {39, "Handshake C++ entry", false, &NetInterfaceHandshakeTestCases::cBoundary},
    {40, "\x54\x4c\x53\xe5\xba\x94\xe7\x94\xa8\xe6\x8f\xa1\xe6\x89\x8b\xe5\xae\x8c\xe6\x95\xb4\xe5\xbe\x80\xe8\xbf\x94", false, &NetInterfaceHandshakeTestCases::tlsPipeline},
    {41, "\xe8\xb4\xa6\xe5\x8f\xb7\xe5\x86\x85\xe9\x83\xa8\xe8\xaf\xb7\xe6\xb1\x82\xe5\x85\xb3\xe8\x81\x94", false, &NetInterfaceRequestsTestCases::nestedRouting},
    {42, "\xe8\xaf\xb7\xe6\xb1\x82\xe9\x99\x90\xe9\xa2\x9d\xe5\x8f\x8a\xe9\x87\x8d\xe5\xa4\x8d\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceRequestsTestCases::budgets},
    {43, "\xe7\x88\xb6\xe5\xad\x90\xe8\xaf\xb7\xe6\xb1\x82\xe6\x88\xaa\xe6\xad\xa2\xe6\x97\xb6\xe9\x97\xb4", false, &NetInterfaceRequestsTestCases::deadlines},
    {44, "\xe5\x8f\x96\xe6\xb6\x88\xe5\x85\x88\xe4\xba\x8e\xe8\xaf\xb7\xe6\xb1\x82\xe5\x88\xb0\xe8\xbe\xbe", false, &NetInterfaceRequestsTestCases::cancellationOrder},
    {45, "\xe7\xab\xaf\xe7\x82\xb9\xe4\xbb\xa3\xe6\xac\xa1\xe5\x8f\x8a\xe5\x85\xb3\xe8\x81\x94\xe5\x9b\x9e\xe6\x94\xb6", false, &NetInterfaceRequestsTestCases::endpointGeneration},
    {46, "\xe8\xaf\xb7\xe6\xb1\x82\xe5\x8f\x82\xe6\x95\xb0\xe5\x8f\x8a\xe5\xae\xb9\xe9\x87\x8f\xe8\xbe\xb9\xe7\x95\x8c", false, &NetInterfaceRequestsTestCases::argumentBounds},
    {47, "Request C++ entry and identity", false, &NetInterfaceRequestsTestCases::cBoundary},
    {57, "Local service credentials and origin wire", false, &NetInterfaceRpcTestCases::serviceWire},
    {48, "\x52\x50\x43\xe5\x9b\xba\xe5\xae\x9a\xe4\xbf\xa1\xe5\xb0\x81\xe5\x8f\x8a\xe5\xae\x8c\xe6\x95\xb4\xe7\xbc\x96\xe5\x8f\xb7", false, &NetInterfaceRpcTestCases::wireVectors},
    {49, "\x52\x50\x43\xe4\xbf\xa1\xe5\xb0\x81\xe6\x8b\x92\xe7\xbb\x9d\xe4\xb8\x8e\xe5\xae\xb9\xe9\x87\x8f", false, &NetInterfaceRpcTestCases::rejection},
    {73, "Portable authenticated link ABI and lifecycle", false, &NetLinkTestCases::portableLink},
    {77, "Negotiated credit window bounds and legacy compatibility", false, &NetLinkTestCases::negotiatedCredit},
    {75, "Portable per-channel credits and forged grant rejection", false, &NetLinkTestCases::creditWindow},
    {70, "Portable opaque routing records and bounds", false, &NetTunnelTestCases::portableRecords},
    {69, "Portable fixed trust and identity CSR", false, &NetIdentityTestCases::portableIdentity},
#ifndef __EMSCRIPTEN__
    {72, "Real gateway control roles and local service enrollment", false, &NetIdentityTestCases::managementAuthentication},
    {76, "Attached endpoint RPC, public origins, children and cancellation", false, &NetEndpointRouterTestCases::attachedEndpoints},
    {83, "Native client lifecycle, callback faults and resources", true, &NetWorkConcurrencyTestCases::lifecycleResources},
    {88, "Queue lifecycle rejects invalid publication without assert", false, &NetInterfaceIpcTestCases::queueLifecycle},
#ifdef _WIN32
    {85, "ProcessWork callback exception isolation", false, &NetInterfaceIpcTestCases::callbackIsolation},
    {86, "ProcessWork ordered slot wait and cursor isolation", false, &NetInterfaceIpcTestCases::readSlotIsolation},
    {87, "ProcessWork initialization bounds and duplicate guard", false, &NetInterfaceIpcTestCases::initializationBounds},
    {89, "ProcessWork notification creation failure rollback", false, &NetInterfaceIpcTestCases::notificationCreationFailure},
#endif
    {84, "libuv loop initialization allocation failure", false, &NetWorkConcurrencyTestCases::loopInitializationFailure},
    {81, "Native host partial creation cleanup", false, &NetWorkConcurrencyTestCases::hostCreationFailures},
    {82, "Paused host fairness and bounded drain", false, &NetWorkConcurrencyTestCases::pausedHostFairness},
    {80, "NetWork CTask concurrent producers over real TCP/TLS", false, &NetWorkConcurrencyTestCases::threadedClients},
    {91, "NetSender preserves actual submission order independent of reservation order", false, &NetGatewayTestCases::senderRequestOrder},
    {92, "Low session callback exception isolation and reusable lifecycle", false, &NetGatewayTestCases::lowSessionCallbackFailures},
    {93, "Partial host creation retains deferred cleanup ownership", false, &NetWorkConcurrencyTestCases::deferredHostCleanup},
    {94, "Closed raw socket tail retains data order and healthy peers", false, &NetWorkConcurrencyTestCases::closedHostTail},
    {95, "Expired synchronous stream frames cannot be sent later", false, &NetWorkConcurrencyTestCases::synchronousFrameExpiry},
    {96, "Local close cancellation differs from remote transport failure", false, &NetGatewayTestCases::lowSessionCloseStatus},
    {97, "NetSender preserves actual request and stream submission order", false, &NetWorkConcurrencyTestCases::submissionOrder},
    {119, "NetSender submission generation and borrowed payload lifetime", false, &NetWorkConcurrencyTestCases::submissionLifetime},
    {120, "Request indexed lifetime and 4096-entry cleanup", false, &NetInterfaceRequestsTestCases::indexedLifetime},
    {121, "IPC inbox full-window fairness and failure isolation", false, &NetInterfaceIpcTestCases::inboxIsolation},
    {122, "IPC Hub receiver ownership and callback registration", false, &NetInterfaceIpcTestCases::hubLifecycle},
    {123, "TLS entropy exception and failed-construction cleanup", false, &NetInterfaceTlsTestCases::entropyExceptionBoundary},
    {124, "TLS frame slot ownership and queue recycling", false, &NetInterfaceConnectionTestCases::recycledQueues},
    {125, "Directory aliases, canonical endpoints and bounded cache", false, &NetInterfaceConnectionTestCases::directoryBoundaries},
    {126, "ProtoMessage atomic containers and independent encoding", false, &NetInterfaceProtoTestCases::atomicContainers},
    {127, "Discovery bounded rotation, canonical failures and restart", false, &NetInterfaceConnectionTestCases::discoverySelection},
    {128, "Gateway channel stop exceptions and bounded OPEN backpressure", false, &NetInterfaceConnectionTestCases::gatewayChannelLifecycle},
    {129, "Recovery lifecycle, discovery faults and callback reentry", false, &NetInterfaceConnectionTestCases::recoveryLifecycle},
    {130, "Recovery fixed total deadline before delayed poll", true, &NetInterfaceConnectionTestCases::recoveryDeadline, 35},
    {131, "Protocol session single request owner, exception and terminal boundaries", false, &NetInterfaceConnectionTestCases::protocolSessionLifecycle},
    {132, "Endpoint router lifecycle, file quota and transactional recovery", false, &NetEndpointRouterTestCases::routerLifecycle},
    {133, "Byte host EOF ordering, bounded events and failure lifecycle", false, &NetWorkConcurrencyTestCases::hostLifecycle},
    {134, "Peer facade generations, invalid reentry and callback lifetime", false, &NetWorkConcurrencyTestCases::peerFacadeLifecycle},
    {135, "Dispatch bounded bidirectional map, generation isolation and backpressure", false, &NetWorkConcurrencyTestCases::dispatchMappingLifecycle},
    {136, "File facade failed results, admission stop and native cleanup", false, &NetWorkConcurrencyTestCases::fileFacadeLifecycle},
    {137, "File protocol codec, resume, duplicate blocks and durable commit", false, &NetWorkConcurrencyTestCases::fileProtocolLifecycle},
    {138, "File transfer completion identity and native file cleanup", false, &NetInterfaceNativeTestCases::fileTransferIdentity},
    {139, "Relay queue capacity, ownership and FIFO over real TCP", false, &netRelayQueueTcpTest},
    {140, "Service queue borrowed capacity, cancellation, deadline and ownership", false, &NetWorkConcurrencyTestCases::serviceQueueOwnership},
    {141, "Service callback reentrant limits, shutdown and exception lifetime", false, &NetSenderServiceQueueTest::reentrant},
    {142, "Service registration atomicity, identity binding and allocation-free cleanup", false, &NetSenderServiceQueueTest::registration},
    {143, "Service gateway phases, failure cleanup and callback reentry", false, &NetWorkConcurrencyTestCases::serviceGatewayLifecycle},
    {144, "Protobuf failed initialization retries without partial publication", false, &NetInterfaceProtoTestCases::initializationRecovery},
    {145, "Service outgoing mapping, exception boundaries and one-shot lifetime", false, &NetWorkConcurrencyTestCases::serviceOutgoingLifecycle},
    {146, "Service proxy bounded FIFO, cancellation and transactional admission", false, &NetWorkConcurrencyTestCases::serviceProxyLifecycle},
    {147, "Service callback clock future timestamp and idle deadline", false, &NetWorkConcurrencyTestCases::serviceCallbackClock},
    {148, "Native host bridge bounded FIFO, independent EOF and overload isolation", false, &NetWorkConcurrencyTestCases::hostBridgeLifecycle},
    {149, "Native temporary file allocation-free cleanup and worker stop", false, &NetWorkConcurrencyTestCases::nativeFileCleanup},
    {150, "String conversion ownership, Unicode and embedded zero", false, &NetWorkConcurrencyTestCases::stringConversion},
    {151, "Libuv accept recovery, peer limit and healthy connection isolation", false, &NetWorkConcurrencyTestCases::libuvAcceptRecovery},
    {152, "Libuv compatibility cold stop and initialized teardown", false, &NetWorkConcurrencyTestCases::libuvLifecycle},
    {153, "NetWork cold close, state and resource lifecycle", false, &NetWorkConcurrencyTestCases::workLifecycle},
    {154, "CString format alias, wide text and growth boundaries", false, &NetWorkConcurrencyTestCases::stringFormat},
    {155, "Partial TLS frame keeps control priority at complete boundaries", false, &NetInterfaceConnectionTestCases::partialFrameControl},
#ifdef _WIN32
    {156, "Process lease ownership, ready state and resource rollback", false, &NetWorkConcurrencyTestCases::processLeaseResources},
#endif
    {98, "NetPeer mixed RPC and authorized DATA arrive in submission order", false, &NetGatewayTestCases::senderMixedOrder},
    {102, "Native file Task stop notification cannot be lost", false, &NetWorkConcurrencyTestCases::fileStopWakeup},
    {103, "Native file Task startup and idle stop resource stress", true, &NetWorkConcurrencyTestCases::fileStopCycles},
    {104, "ProcessWork raw send/post invalid lengths preserve recovery", false, &NetInterfaceIpcTestCases::invalidSendBounds},
    {105, "Owned temporary file completes durable transfer and atomic publish", false, &NetInterfaceNativeTestCases::ownedTemporaryTransfer},
    {106, "IPC ciphertext ownership and 20000 ordered full-size packet stress", true, &NetWorkConcurrencyTestCases::ipcInputOwnership},
    {107, "Service backpressure preserves FIFO across poll budgets and healthy sources", false, &NetSenderServiceQueueTest::run},
    {108, "Existing writable temporary file resumes hashing and atomic publish", false, &NetInterfaceNativeTestCases::resumedFileTransfer},
    {109, "Cancelled close and publish release native handles before backend shutdown", false, &NetWorkConcurrencyTestCases::fileCancelledRelease},
    {111, "\x49\x50\x43\xe5\xa3\xb0\xe6\x98\x8e\xe5\xae\xb9\xe9\x87\x8f\xe4\xb8\x8e\xe6\x89\x93\xe5\xbc\x80\xe6\x8b\x92\xe7\xbb\x9d", false, &NetInterfaceIpcTestCases::ipcCapacity},
    {112, "\xe6\x97\xa2\xe6\x9c\x89\xe6\x8e\xa5\xe6\x94\xb6\xe5\x99\xa8\xe6\x89\x80\xe6\x9c\x89\xe6\x9d\x83\xe4\xbf\x9d\xe6\x8a\xa4", false, &NetInterfaceIpcTestCases::receiverOwnership},
    {113, "\xe5\xa4\x96\xe9\x83\xa8\xe6\x8e\xa5\xe6\x94\xb6\xe5\x99\xa8\xe6\x9c\x80\xe5\xb0\x8f\xe5\xae\xb9\xe9\x87\x8f\xe5\x8f\x8a\xe5\xa4\xb1\xe8\xb4\xa5\xe9\x87\x8d\xe8\xaf\x95", false, &NetInterfaceIpcTestCases::externalReceiverCapacity},
    {114, "\xe6\x9c\xac\xe5\x9c\xb0\x54\x4c\x53\xe5\x8f\x8a\xe9\x80\x8f\xe6\x98\x8e\xe6\x8e\xa5\xe5\x85\xa5\xe6\x9c\x80\xe5\xb0\x8f\xe5\xae\xb9\xe9\x87\x8f", false, &NetInterfaceIpcTestCases::localReceiverCapacity},
    {115, "\xe6\x8e\xa5\xe6\x94\xb6\xe5\x88\x9d\xe5\xa7\x8b\xe5\x8c\x96\xe5\xa4\xb1\xe8\xb4\xa5\xe4\xbf\x9d\xe7\x95\x99\x50\x6f\x73\x74\xe5\x8f\x8a\xe9\x87\x8d\xe8\xaf\x95\xe5\x9b\x9e\xe8\xb0\x83", false, &NetInterfaceIpcTestCases::managedReceiverRollback},
    {101, "NetWork owner notification cannot be lost before wait", false, &NetWorkConcurrencyTestCases::ownerWakeup},
    {99, "NetWork concurrent send/post preserves every producer arrival order", false, &NetWorkConcurrencyTestCases::threadedOrder},
    {90, "NetPeer send/post request and stream handoff over real TLS", false, &NetGatewayTestCases::senderDelivery},
    {187, "Main factories preserve ownership and independent sender state", false, &NetWorkInProcessTestCases::mainFactories},
    {188, "Singleton facade desktopCpp independent clients and boundaries", false, &NetSenderFacadeNetSenderCppTest::desktopCpp},
    {189, "C++ entry isolation and host boundary contracts", false, &NetCppBoundaryProbe::cppBoundary},
    {190, "Singleton facade peerCpp independent clients and boundaries", false, &NetSenderFacadeNetPeerCppTest::peerCpp},
    {191, "Singleton facade peerC independent clients and boundaries", false, &NetSenderFacadeNetPeerCTest::peerC},
    {192, "Singleton facade workCpp independent clients and boundaries", false, &NetSenderFacadeNetWorkCppTest::workCpp},
    {193, "Singleton facade workC independent clients and boundaries", false, &NetSenderFacadeNetWorkCTest::workC},
    {194, "Shared C facade multi-producer TCP/TLS and per-thread FIFO", false, &NetSenderFacadeConcurrency::workConcurrency},
    {79, "In-process C++ NetSender over TCP for desktop/mobile/mini-program platforms", false, &NetWorkInProcessTestCases::clientPlatforms},
    {78, "\xe8\xb6\x85\xe6\x97\xb6\xe5\x9b\x9e\xe8\xb0\x83\xe6\x8f\x90\xe4\xba\xa4\xe6\x96\xb0\xe8\xaf\xb7\xe6\xb1\x82\xe4\xb8\x8e\xe5\x8d\x95\xe8\xb0\x83\xe6\x97\xb6\xe9\x92\x9f", false, &NetGatewayTestCases::sessionCallbackDeadline},
    {71, "Endpoint TLS through two real opaque TCP segments", false, &netRelayTlsTcpTest},
    {68, "Authorized identity issuance and leaf TLS rotation", false, &NetIdentityTestCases::authorityAndTls},
#endif
    {100, "\xe6\x9c\x89\xe7\x95\x8c\xe5\x8e\x8b\xe5\x8a\x9b", true, &NetWorkTestCases::stress}
};

/** 信号处理器只写标准信号标志，不执行I/O或清理
@param [in] signalNumber 系统中断信号
*/
static void requestCancel(int signalNumber)
{
    (void)signalNumber;
    g_cancelRequested = 1;
}

void netTestCheckpoint()
{
    if (g_cancelRequested != 0)
    {
        throw std::runtime_error("CANCELLED");
    }
    if (std::chrono::steady_clock::now() >= g_deadline)
    {
        throw std::runtime_error("TIMEOUT");
    }
}

/** 解析严格正整数，不接受尾随垃圾或溢出
@param [in] text ASCII数字
@param [in] maximum 此入口接受的最大正整数
@return 合法编号；不合法抛出参数异常
*/
static int parseId(const std::string& text, unsigned long maximum = 10000)
{
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
    {
        throw std::runtime_error("invalid case ID");
    }
    unsigned long value = std::strtoul(text.c_str(), nullptr, 10);
    if (value == 0 || value > maximum)
    {
        throw std::runtime_error("case ID out of range");
    }
    return static_cast<int>(value);
}

/** 解析逗号和范围选项，非法输入不退回全量
@param [in] text 例如1,3-5
@param [out] ids 去重编号集合
*/
static void parseSelection(const std::string& text, std::set<int>& ids)
{
    if (text.empty() || text[text.size() - 1] == ',')
    {
        throw std::runtime_error("empty selection");
    }
    std::istringstream input(text);
    std::string part;
    while (std::getline(input, part, ','))
    {
        size_t dash = part.find('-');
        int first = parseId(part.substr(0, dash));
        int last = dash == std::string::npos ? first : parseId(part.substr(dash + 1));
        if (last < first)
        {
            throw std::runtime_error("reversed range");
        }
        for (int id = first; id <= last; ++id)
        {
            ids.insert(id);
        }
    }
}

/** 为CSV引用文本并转义双引号
@param [in] value 待写UTF-8文本
@return 一个完整CSV字段
*/
static std::string csv(const std::string& value)
{
    std::string result = "\"";
    for (size_t index = 0; index < value.size(); ++index)
    {
        if (value[index] == '"')
        {
            result += '"';
        }
        result += value[index];
    }
    return result + "\"";
}

/** 显式故障探针只验证runner，不登记为产品通过
@param [in] probe fail、timeout或cancel
*/
static void runnerProbe(const std::string& probe)
{
    if (probe == "timeout")
    {
        g_deadline = std::chrono::steady_clock::now();
    }
    else if (probe == "cancel")
    {
        g_cancelRequested = 1;
    }
    netTestCheckpoint();
    throw std::runtime_error("INJECTED_FAILURE");
}

#ifndef __EMSCRIPTEN__
/** 本程序入口将LibuvTcp接入通用宿主表；NetWork不选择具体实现
@return NI_OK或错误码
*/
static int32_t ConfigureGatewayHost()
{
    NetHostFunctions functions = {};
    functions.structSize = sizeof(functions);
    functions.version = 1;
    functions.create = LibuvTcp::hostCreate;
    functions.listen = LibuvTcp::hostListen;
    functions.connect = LibuvTcp::hostConnect;
    functions.poll = LibuvTcp::hostPoll;
    functions.write = LibuvTcp::hostWrite;
    functions.pauseRead = LibuvTcp::hostPauseRead;
    functions.close = LibuvTcp::hostClose;
    functions.shutdown = LibuvTcp::hostShutdown;
    functions.destroy = LibuvTcp::hostDestroy;
    functions.localPort = LibuvTcp::hostLocalPort;
    functions.peerCount = LibuvTcp::hostPeerCount;
    functions.entropy = CSystem::secureRandomBytesCallback;
    return NetWork::configureHost(functions);
}

#endif

/** 运行指定测试并落盘；没有选择时仅列用法
@param [in] argc 命令行项数
@param [in] argv 参数，包括--cases/--instant/--stress/--all及--report
@return 0通过，1测试失败/取消，2参数错误，3报告失败
*/
int main(int argc, char** argv)
{
#ifndef __EMSCRIPTEN__
    if (ConfigureGatewayHost() != NI_OK || NetWork::configureNativeFileIo() != NI_OK)
    {
        return 4;
    }
#endif
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    if (argc == 3 && (std::string(argv[1]) == "--link-input-audit" || std::string(argv[1]) == "--sender-link-audit" ||
        std::string(argv[1]) == "--link-output-audit" || std::string(argv[1]) == "--link-auth-audit"))
    {
        try
        {
            if (std::string(argv[1]) == "--link-auth-audit")
            {
                return NetLinkTestCases::authenticationAudit(argc - 1, argv + 1);
            }
            if (std::string(argv[1]) == "--link-output-audit")
            {
                return NetLinkTestCases::outputQueueAudit(argc - 1, argv + 1);
            }
            return std::string(argv[1]) == "--link-input-audit" ? NetLinkTestCases::inputDrainAudit(argc - 1, argv + 1) :
                NetLinkTestCases::senderDrainAudit(argc - 1, argv + 1);
        }
        catch (...)
        {
            return 98;
        }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 12, "--ipc-audit=") == 0)
    {
        g_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        try { return NetInterfaceIpcTestCases::auditChild(std::string(argv[1]).substr(12)); }
        catch (...) { return 97; }
    }
    if (argc == 2 && std::string(argv[1]) == "--client-launch-probe")
    {
        try
        {
            uint32_t pid = 0;
            const int32_t result = NetSenderGatewayLaunch::ensure(pid);
            std::printf("CLIENT_LAUNCH result=%d pid=%u\n", result, pid);
            return result == NI_OK || result == NI_NOT_FOUND ? 0 : 1;
        }
        catch (...) { return 1; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 22, "--unavailable-service=") == 0)
    {
        try { return NetGatewayClientTestProbe::unavailableMain(std::string(argv[1]).substr(22)); }
        catch (...) { return 67; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 18, "--recovery-native=") == 0)
    {
        try { return NetGatewayClientTestProbe::recoveryMain(std::string(argv[1]).substr(18), false); }
        catch (...) { return 66; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 19, "--recovery-desktop=") == 0)
    {
        try { return NetGatewayClientTestProbe::recoveryMain(std::string(argv[1]).substr(19), true); }
        catch (...) { return 66; }
    }
    if (argc == 2 && (std::string(argv[1]).compare(0, 15, "--file-service=") == 0 ||
        std::string(argv[1]).compare(0, 14, "--file-client=") == 0))
    {
        try
        {
            const bool server = std::string(argv[1]).compare(0, 15, "--file-service=") == 0;
            return NetFileGatewayProbe::run(server, parseId(std::string(argv[1]).substr(server ? 15 : 14), 2147483647UL));
        }
        catch (const std::exception& error) { std::fprintf(stderr, "file gateway probe: %s\n", error.what()); return 84; }
        catch (...) { return 84; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 17, "--account-client=") == 0)
    {
#ifdef NET_WORK_TEST_ACCOUNT
        try { return NetAccountGatewayProbe::run(parseId(std::string(argv[1]).substr(17), 2147483647UL)); }
        catch (const std::exception& error) { std::fprintf(stderr, "account client: %s\n", error.what()); return 73; }
        catch (...) { return 73; }
#else
        std::fprintf(stderr, "Account integration requires an Account SDK rebuilt for this NetSender API and NET_WORK_TEST_ACCOUNT.\n");
        return 73;
#endif
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 17, "--desktop-client=") == 0)
    {
        try { return NetServiceTestCases::desktopClientMain(parseId(std::string(argv[1]).substr(17), 2147483647UL)); }
        catch (...) { return 55; }
    }
    if (argc == 3 && std::string(argv[1]).compare(0, 21, "--standalone-service=") == 0 &&
        std::string(argv[2]).compare(0, 18, "--service-product=") == 0)
    {
        try
        {
            const std::string product = std::string(argv[2]).substr(18);
            if (product != "0" && product != "4") return 56;
            return NetServiceTestCases::standaloneMain(parseId(std::string(argv[1]).substr(21), 2147483647UL),
                product == "0" ? 0 : 4);
        }
        catch (const std::exception& error) { std::fprintf(stderr, "standalone service: %s\n", error.what()); return 57; }
        catch (...) { return 57; }
    }
    if (argc == 4 && std::string(argv[1]).compare(0, 16, "--service-child=") == 0 &&
        std::string(argv[2]).compare(0, 18, "--service-product=") == 0 &&
        std::string(argv[3]).compare(0, 15, "--service-mode=") == 0)
    {
        try
        {
            const std::string product = std::string(argv[2]).substr(18);
            return NetServiceTestCases::childMain(parseId(std::string(argv[1]).substr(16), 2147483647UL),
                product == "0" ? 0 : parseId(product, 4), parseId(std::string(argv[3]).substr(15), 3));
        }
        catch (...)
        {
            return 46;
        }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 18, "--raw-local-child=") == 0)
    {
        try { return NetRawLocalTestCases::childMain(parseId(std::string(argv[1]).substr(18), 2147483647UL)); }
        catch (const std::exception& error) { std::fprintf(stderr, "raw local child: %s\n", error.what()); return 1; }
        catch (...) { return 1; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 21, "--service-grant-test=") == 0)
    {
        try { return NetLinkLocalTestProbe::serviceGrantChildMain(parseId(std::string(argv[1]).substr(21), 2147483647UL)); }
        catch (...) { return 1; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 21, "--managed-local-test=") == 0)
    {
        try { return NetLinkLocalTestProbe::childMain(parseId(std::string(argv[1]).substr(21), 2147483647UL)); }
        catch (...) { return 1; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 22, "--service-client-test=") == 0)
    {
        try { return NetGatewayClientTestProbe::childMain(parseId(std::string(argv[1]).substr(22), 2147483647UL)); }
        catch (...) { return 1; }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 24, "--facade-service-client=") == 0)
    {
        try
        {
            const std::string pid = std::string(argv[1]).substr(24);
            return NetGatewayClientTestProbe::facadeMain(pid == "0" ? 0 : parseId(pid, 2147483647UL));
        }
        catch (...) { return 1; }
    }
    if (argc == 3 && std::string(argv[1]).compare(0, 14, "--local-child=") == 0 &&
        std::string(argv[2]).compare(0, 13, "--local-mode=") == 0)
    {
        try
        {
            return NetLocalTestCases::childMain(parseId(std::string(argv[1]).substr(14), 2147483647UL),
                parseId(std::string(argv[2]).substr(13), 3));
        }
        catch (...)
        {
            return 25;
        }
    }
    if (argc == 2 && std::string(argv[1]).compare(0, 12, "--ipc-child=") == 0)
    {
        try
        {
            return NetInterfaceIpcTestCases::childMain(parseId(std::string(argv[1]).substr(12), 2147483647UL));
        }
        catch (...)
        {
            return 12;
        }
    }
#endif
    std::signal(SIGINT, requestCancel);
    std::signal(SIGTERM, requestCancel);
    std::set<int> ids;
    std::string mode;
    std::string reportPath;
    std::string probe;
    bool list = false;
    try
    {
        for (int index = 1; index < argc; ++index)
        {
            std::string arg(argv[index]);
            if (arg == "--list")
            {
                if (list) throw std::runtime_error("duplicate list option");
                list = true;
            }
            else if (arg.compare(0, 9, "--report=") == 0)
            {
                if (!reportPath.empty() || arg.size() == 9)
                    throw std::runtime_error("empty or duplicate report option");
                reportPath = arg.substr(9);
            }
            else if (arg.compare(0, 15, "--runner-probe=") == 0)
            {
                if (!probe.empty()) throw std::runtime_error("duplicate runner probe");
                probe = arg.substr(15);
                if (probe != "fail" && probe != "timeout" && probe != "cancel")
                {
                    throw std::runtime_error("unknown runner probe");
                }
            }
            else if (arg.compare(0, 8, "--cases=") == 0)
            {
                if (!mode.empty())
                {
                    throw std::runtime_error("choose one selection mode");
                }
                mode = "cases";
                parseSelection(arg.substr(8), ids);
            }
            else if (arg == "--instant" || arg == "--stress" || arg == "--all")
            {
                if (!mode.empty())
                {
                    throw std::runtime_error("choose one selection mode");
                }
                mode = arg;
            }
            else
            {
                throw std::runtime_error("unknown argument: " + arg);
            }
        }
        if (list && !mode.empty()) throw std::runtime_error("list must be isolated");
        if (!probe.empty() && (!mode.empty() || list))
        {
            throw std::runtime_error("runner probe must be isolated");
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 2;
    }
    std::vector<const NetWorkTestEntry*> selected;
    for (size_t index = 0; index < sizeof(g_tests) / sizeof(g_tests[0]); ++index)
    {
        const NetWorkTestEntry& entry = g_tests[index];
        if (list || (mode == "cases" && ids.count(entry.m_id) != 0) || mode == "--all" ||
            (mode == "--instant" && !entry.m_stress) || (mode == "--stress" && entry.m_stress))
        {
            selected.push_back(&entry);
        }
    }
    std::sort(selected.begin(), selected.end(), [](const NetWorkTestEntry* left, const NetWorkTestEntry* right) {
        return left->m_id < right->m_id;
    });
    if (list)
    {
        for (size_t index = 0; index < selected.size(); ++index)
            std::cout << selected[index]->m_id << " " << selected[index]->m_name << " "
                << (selected[index]->m_stress ? "stress" : "instant") << std::endl;
        return 0;
    }
    if ((mode == "cases" && selected.size() != ids.size()) ||
        (selected.empty() && probe.empty()))
    {
        std::cerr << "Use --list, --cases=1,3-5, --instant, --stress or --all." << std::endl;
        return 2;
    }
    std::ofstream report;
    if (!reportPath.empty())
    {
        report.open(reportPath.c_str(), std::ios::binary | std::ios::trunc);
        if (!report)
        {
            std::cerr << "Cannot open report." << std::endl;
            return 3;
        }
        report << "id,name,state,duration_ms,detail\n";
    }
    size_t passed = 0;
    size_t failed = 0;
    size_t cancelled = 0;
    size_t skipped = 0;
    size_t total = probe.empty() ? selected.size() : 1;
    for (size_t index = 0; index < total; ++index)
    {
        int id = probe.empty() ? selected[index]->m_id : 9001;
        const char* name = probe.empty() ? selected[index]->m_name : "runner probe";
        bool stress = probe.empty() && selected[index]->m_stress;
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        const uint32_t timeoutSeconds = probe.empty() && selected[index]->m_timeoutSeconds != 0 ?
            selected[index]->m_timeoutSeconds : (stress ? 30U : 10U);
        g_deadline = start + std::chrono::seconds(timeoutSeconds);
        std::string state = "PASS";
        std::string detail;
        if (g_cancelRequested != 0)
        {
            state = "NOT_RUN";
            detail = "cancelled before start";
            ++skipped;
        }
        else
        {
            try
            {
                if (probe.empty())
                {
                    selected[index]->m_run();
                }
                else
                {
                    runnerProbe(probe);
                }
                netTestCheckpoint();
                ++passed;
            }
            catch (const std::exception& error)
            {
                detail = error.what();
                if (g_cancelRequested != 0 && detail == "CANCELLED")
                {
                    state = "CANCELLED";
                    ++cancelled;
                }
                else
                {
                    state = "FAIL";
                    ++failed;
                }
            }
            catch (...)
            {
                state = "FAIL";
                detail = "unknown exception";
                ++failed;
            }
        }
        long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        std::cout << id << " " << name << " " << state << " " << elapsed << "ms " << detail << std::endl;
        if (report.is_open())
        {
            report << id << "," << csv(name) << "," << state << "," << elapsed << "," << csv(detail) << "\n";
            report.flush();
            if (!report)
            {
                return 3;
            }
        }
    }
    std::cout << "TOTAL=" << total << " PASS=" << passed << " FAIL=" << failed
        << " CANCELLED=" << cancelled << " NOT_RUN=" << skipped << std::endl;
    if (report.is_open())
    {
        report << "TOTAL," << total << ",PASS," << passed << ",FAIL," << failed
            << ",CANCELLED," << cancelled << ",NOT_RUN," << skipped << "\n";
        report.close();
        if (!report)
        {
            return 3;
        }
    }
    return failed == 0 && cancelled == 0 && skipped == 0 ? 0 : 1;
}