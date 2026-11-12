#include "LibuvChannelModels.h"
#include "LibuvChannelState.h"
#include <new>

LibuvChannelPeer::LibuvChannelPeer(LibuvChannelState* owner, uint64_t id) :
m_owner(owner),
m_id(id),
m_queuedBytes(0),
m_queuedWrites(0),
m_reason(0),
m_connected(false),
m_closing(false),
m_paused(false),
m_reportClose(id != 0),
m_input(new (std::nothrow) uint8_t[owner->m_readBufferBytes])
{

}

LibuvChannelPeer::~LibuvChannelPeer()
{
    delete[] m_input;
}

LibuvChannelWrite::LibuvChannelWrite() :
m_peer(nullptr),
m_next(nullptr)
{
    m_request.data = this;
}