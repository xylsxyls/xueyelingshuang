#include "NetIpcAdapterFixture.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>

/** 核对独立预期，失败交给runner报告
@param [in] condition 预期条件
@param [in] reason 失败原因
*/
static void IpcFixtureCheck(bool condition, const char* reason)
{
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

NetIpcAdapterFixture::NetIpcAdapterFixture() :
m_full(0)
{

}

NetIpcAdapterFixture::~NetIpcAdapterFixture()
{
    m_sender.close();
    m_receiver.close();
    char* packet = nullptr;
    while (m_packets.pop(&packet))
    {
        delete[] packet;
    }
}

void NetIpcAdapterFixture::open(int32_t slots)
{
    const int32_t pid = CSystem::currentProcessPid();
    IpcFixtureCheck(m_receiver.openReceiver(4096, slots, 0, 0, 0, 0), "IPC receiver create");
    m_receiver.publish();
    IpcFixtureCheck(m_sender.open(pid, pid, 8192) == NET_SENDER_IPC_TOO_LARGE, "IPC declared limit rejected");
    IpcFixtureCheck(m_sender.open(pid, pid, 4084) == NET_SENDER_IPC_ACCEPTED, "IPC sender open");
    int32_t result = NET_SENDER_IPC_RESOURCE;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (NetSenderIpcChannel::retryable(result) && std::chrono::steady_clock::now() < deadline)
    {
        m_receiver.pump(m_packets, m_delivered, m_full);
        result = m_sender.trySend(nullptr, 0, 0);
    }
    IpcFixtureCheck(result == NET_SENDER_IPC_ACCEPTED, "IPC registration deadline");
    IpcFixtureCheck(m_receiver.pump(m_packets, m_delivered, m_full) == 1, "IPC registration publication");
    checkPacket(nullptr, 0, 0);
}

void NetIpcAdapterFixture::checkPacket(const uint8_t* bytes, int32_t size, int32_t type)
{
    char* packet = nullptr;
    IpcFixtureCheck(m_packets.pop(&packet) && packet != nullptr, "IPC packet missing");
    std::unique_ptr<char[]> owned(packet);
    const int32_t header[] = {CSystem::currentProcessPid(), type, size};
    IpcFixtureCheck(std::memcmp(packet, header, sizeof(header)) == 0 &&
        (size == 0 || std::memcmp(packet + sizeof(header), bytes, size) == 0), "IPC exact header and body");
}