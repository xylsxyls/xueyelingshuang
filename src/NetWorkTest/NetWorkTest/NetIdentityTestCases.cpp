#include "NetIdentityTestCases.h"
#include "NetGatewayAuthorityTest.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterfaceTestAccess.h"
#include <cstring>
#ifndef __EMSCRIPTEN__
#include "CSystem/CSystemAPI.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <chrono>
#endif
#endif

void NetIdentityTestCases::portableIdentity()
{
    uint32_t size = 0;
    const uint8_t* authority = NetInterface::trustAuthority(&size);
    NetInterfaceTlsTestPair::require(authority != nullptr && size > 100 && authority[size - 1] == 0,
        "compiled authority must be a stable NUL-terminated certificate");
    const std::string root(reinterpret_cast<const char*>(authority), size - 1);
    NetInterfaceTlsTestPair::require(root.find("PRIVATE KEY") == std::string::npos, "public root contains a private key");
    NetInterfaceTlsTestPair::require(!NetInterfaceCertificate::validName("service.1,CN=node.evil") &&
        !NetInterfaceCertificate::validName("service..1") && !NetInterfaceCertificate::validName("*.service") &&
        !NetInterfaceCertificate::validName("node.-bad") && !NetInterfaceCertificate::validName(std::string(97, 'a')),
        "unsafe identity name was accepted");
    std::string key;
    std::string request;
    NetInterfaceTlsTestPair::require(NetInterfaceCertificate::createRequest("service.1", NetInterfaceTlsTestPair::strongRandom,
        nullptr, key, request) == NI_OK, "portable P256 CSR creation failed");
    NetInterfaceTlsTestPair::require(key.find("PRIVATE KEY") != std::string::npos &&
        request.find("CERTIFICATE REQUEST") != std::string::npos && request.find("PRIVATE KEY") == std::string::npos,
        "CSR/private-key ownership boundary invalid");
    std::string otherKey;
    std::string otherRequest;
    NetInterfaceTlsTestPair::require(NetInterfaceCertificate::createRequest("service.1", NetInterfaceTlsTestPair::strongRandom,
        nullptr, otherKey, otherRequest) == NI_OK && key != otherKey, "new identities reused the private key");
    NetInterfaceTlsTestPair::require(NetInterfaceCertificate::createRequest("service.1", NetInterfaceTlsTestPair::failedRandom,
        nullptr, key, request) != NI_OK && key.empty() && request.empty(), "entropy failure did not clear outputs");
}

void NetIdentityTestCases::authorityAndTls()
{
    NetInterfaceTlsTestPair::require(NetGatewayAuthorityTest::identity() == NI_OK, "gateway identity audit failed");
}

void NetIdentityTestCases::managementAuthentication()
{
    NetInterfaceTlsTestPair::require(NetGatewayAuthorityTest::management() == NI_OK, "gateway management audit failed");
}