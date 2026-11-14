#include "LibuvChannelModels.h"
#include "LibuvChannelState.h"
#include <new>

LibuvChannelPeer::LibuvChannelPeer(LibuvChannelState* owner, uint64_t id) :
m_storage(nullptr),
m_postFirst(nullptr),
m_postLast(nullptr),
m_readyPrevious(nullptr),
m_readyNext(nullptr),
m_readyQueued(false),
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
m_peerId(0),
m_size(0),
m_storage(nullptr),
m_peer(nullptr),
m_next(nullptr)
{
    m_request.data = this;
}

uint8_t* LibuvChannelWrite::data()
{
    return m_size <= sizeof(m_inline) ? m_inline : m_bytes.data();
}

LibuvChannelLane::LibuvChannelLane() :
m_token(0), m_capacity(0), m_published(0), m_write(0), m_seenRead(0), m_peer(0), m_version(0),
m_notified(false), m_consumed(0), m_read(0)
{

}