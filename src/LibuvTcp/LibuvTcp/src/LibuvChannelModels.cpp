#include "LibuvChannelModels.h"

LibuvChannelPeer::LibuvChannelPeer(LibuvChannelState* owner, uint64_t id) :
m_owner(owner),
m_id(id),
m_queuedBytes(0),
m_queuedWrites(0),
m_reason(0),
m_connected(false),
m_closing(false),
m_paused(false),
m_reportClose(id != 0)
{

}

LibuvChannelWrite::LibuvChannelWrite(LibuvChannelPeer* peer, const uint8_t* data, uint32_t size) :
m_peer(peer),
m_bytes(data, data + size)
{
    m_request.data = this;
}