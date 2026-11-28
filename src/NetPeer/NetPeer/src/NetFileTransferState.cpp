#include "NetFileTransferState.h"

NetFileTransferState::NetFileTransferState() :
m_io(nullptr),
m_file(0),
m_waiting(),
m_job(),
m_prime(0),
m_batchData(nullptr), m_batchOffset(0), m_batchSize(0),
m_resume(0),
m_now(0),
m_finalOutputSince(UINT64_MAX),
m_phase(NET_FILE_TRANSFER_DATA),
m_failure(NI_OK),
m_sender(false),
m_replace(false)
{

}

NetFileTransferState::~NetFileTransferState()
{
    NetInterface::wipe(m_temporary.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_temporary[0]),
        static_cast<uint32_t>(m_temporary.size()));
    NetInterface::wipe(m_destination.empty() ? nullptr : reinterpret_cast<uint8_t*>(&m_destination[0]),
        static_cast<uint32_t>(m_destination.size()));
}