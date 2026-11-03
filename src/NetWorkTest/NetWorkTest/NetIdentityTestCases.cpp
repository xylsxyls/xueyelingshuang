#include "NetIdentityTestCases.h"
#include "NetWorkTestCases.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetInterface/NetInterfaceAPI.h"
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

/** 在隔离子进程中运行指定网关身份验证场景
@param [in] selector 所选隔离身份测试场景，不能为空
*/
static void RunGatewayIdentityTest(const char* selector)
{
#ifndef __EMSCRIPTEN__
#ifdef _WIN32
    wchar_t ownPath[32768] = {};
    const DWORD count = GetModuleFileNameW(nullptr, ownPath, 32768);
    NetInterfaceTlsTestPair::require(count != 0 && count < 32768, "identity test executable path failed");
    std::wstring server(ownPath, count);
    const size_t separator = server.find_last_of(L"\\/");
    NetInterfaceTlsTestPair::require(separator != std::wstring::npos, "identity server directory missing");
    server.resize(separator + 1);
    const std::wstring directory = server;
    server += L"NetServerManager1.1.exe";
    if (GetFileAttributesW(server.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        server = directory + L"..\\NetServerManager\\NetServerManager1.1.exe";
    }
    std::wstring command = L"\"" + server + L"\" " + std::wstring(selector, selector + std::strlen(selector));
    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child = {};
    NetInterfaceTlsTestPair::require(CreateProcessW(server.c_str(), &command[0], nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child) != FALSE, "start real gateway identity self-test failed");
    CloseHandle(child.hThread);
    const DWORD waited = WaitForSingleObject(child.hProcess, 10000);
    DWORD exitCode = 1;
    if (waited == WAIT_OBJECT_0)
    {
        GetExitCodeProcess(child.hProcess, &exitCode);
    }
    else
    {
        TerminateProcess(child.hProcess, 1);
        WaitForSingleObject(child.hProcess, 2000);
    }
    CloseHandle(child.hProcess);
    NetInterfaceTlsTestPair::require(waited == WAIT_OBJECT_0 && exitCode == 0, "real gateway identity self-test failed");
#else
    std::string server = CSystem::joinPath(CSystem::GetCurrentExePath(), "NetServerManager1.1");
    if (access(server.c_str(), X_OK) != 0)
    {
        server = CSystem::joinPath(CSystem::GetCurrentExePath(), "../NetServerManager/NetServerManager1.1");
    }
    const pid_t child = fork();
    NetInterfaceTlsTestPair::require(child >= 0, "fork real gateway identity self-test failed");
    if (child == 0)
    {
        execl(server.c_str(), server.c_str(), selector, static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    pid_t result = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (result == 0 && std::chrono::steady_clock::now() < deadline)
    {
        result = waitpid(child, &status, WNOHANG);
        if (result == 0) { CSystem::Sleep(1); }
    }
    if (result == 0)
    {
        kill(child, SIGKILL);
        waitpid(child, &status, 0);
    }
    NetInterfaceTlsTestPair::require(result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
        "real gateway identity self-test failed");
#endif
#endif
}

void NetIdentityTestCases::authorityAndTls()
{
    RunGatewayIdentityTest("--self-test=identity");
}

void NetIdentityTestCases::managementAuthentication()
{
    RunGatewayIdentityTest("--self-test=link");
}