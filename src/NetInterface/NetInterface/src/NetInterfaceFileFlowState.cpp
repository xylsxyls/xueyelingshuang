#include "NetInterfaceFileFlowState.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceFileGuard.h"
#include <algorithm>
#include <cstring>
#include "NetInterface.h"

NetInterfaceFileFlowOpaque::NetInterfaceFileFlowOpaque(const NetInterfaceFileInfo& info, uint32_t product, bool sender, uint64_t nowMs,
                                                       uint32_t timeoutMs) :
m_flow(info, product, sender, nowMs, timeoutMs)
{

}

NetInterfaceFileFlowState::NetInterfaceFileFlowState(const NetInterfaceFileInfo& info, uint32_t product, bool sender,
    uint64_t nowMs, uint32_t timeoutMs) :
m_info(info),
m_hash(nullptr),
m_phase(info.offset == 0 ? NI_FILE_PHASE_TRANSFER : NI_FILE_PHASE_PREFIX),
m_outputKind(NI_FILE_OUTPUT_NONE),
m_primed(0),
m_position(info.offset),
m_acknowledged(info.offset),
m_now(nowMs),
m_progress(nowMs),
m_outputAckOffset(0),
m_product(product),
m_timeout(timeoutMs),
m_outputSize(0),
m_writeBatchChunks(1),
m_outputAckFlags(0),
m_error(NI_OK),
m_sender(sender),
m_writeTaken(false),
m_ackPending(false)
{
    uint8_t probe[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NetInterfaceFileGuard wipe(probe, sizeof(probe));
    if (product == 0 || timeoutMs < 1000 || timeoutMs > 60000 || info.state != NI_FILE_TRANSFERRING ||
        NetInterfaceFile::encodeInfo(info, probe, sizeof(probe)) != NI_OK)
    {
        m_error = NI_INVALID_ARGUMENT;
    }
    else
    {
        m_error = NetInterface::hashCreate(&m_hash);
    }
    if (m_error != NI_OK)
    {
        m_phase = NI_FILE_PHASE_FAILED;
    }
}

NetInterfaceFileFlowState::~NetInterfaceFileFlowState()
{
    clearData();
    clearOutput();
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_info), sizeof(m_info));
    NetInterface::hashDestroy(m_hash);
}

void NetInterfaceFileFlowState::clearData()
{
    NetInterface::wipe(m_data.empty() ? nullptr : m_data.data(), static_cast<uint32_t>(m_data.size()));
    m_data.clear();
    m_writeTaken = false;
}

void NetInterfaceFileFlowState::clearOutput()
{
    NetInterface::wipe(m_output.empty() ? nullptr : m_output.data(), static_cast<uint32_t>(m_output.size()));
    m_output.clear();
    m_outputKind = NI_FILE_OUTPUT_NONE;
    m_outputSize = 0;
}

void NetInterfaceFileFlowState::prepareOutput(uint32_t size)
{
    // 只在空输出槽使用；在写入秘密之前一次预留本角色的最大帧。
    m_output.reserve(m_sender ? NET_INTERFACE_FILE_DATA_HEADER_SIZE + m_info.chunk : NET_INTERFACE_FILE_ACK_SIZE);
    m_output.resize(size);
}

int32_t NetInterfaceFileFlowState::status() const
{
    return m_phase == NI_FILE_PHASE_FAILED ? m_error : m_phase == NI_FILE_PHASE_DONE ? NI_READY : NI_OK;
}