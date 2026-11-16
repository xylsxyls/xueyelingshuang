#include "NetFileIoTest.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <cstring>
#include <vector>

uint32_t NetFileIoTestBackend::m_mode = 0;
uint32_t NetFileIoTestBackend::m_destroyed = 0;
NetFileIoTestBackend* NetFileIoTestBackend::m_current = nullptr;
NetFileIo* NetFileIoTestBackend::m_owner = nullptr;

NetFileIoTestBackend::NetFileIoTestBackend() :
m_next(0),
m_submitted(0),
m_throw(false),
m_stopped(false),
m_done(false)
{
    m_current = this;
}

NetFileIoTestBackend::~NetFileIoTestBackend()
{
    ++m_destroyed;
}

int32_t NetFileIoTestBackend::init()
{
    if (m_mode == 5) { m_done = true; return NI_INVALID_ARGUMENT; }
    if (m_mode == 6) { m_done = true; throw 6; }
    if (m_mode == 19) { m_owner->stop(); }
    return NI_OK;
}

int32_t NetFileIoTestBackend::nextRequestId(uint64_t& id)
{
    id = ++m_next;
    if (m_mode == 4) { return NI_LIMIT_EXCEEDED; }
    if (m_mode == 7) { id = 0; return NI_OK; }
    if (m_throw)
    {
        throw 1;
    }
    return NI_OK;
}

int32_t NetFileIoTestBackend::submit(const NetFileIoRequest& request)
{
    (void)request;
    ++m_submitted;
    if (m_mode == 8) { throw 8; }
    if (m_mode == 9) { return NI_READY; }
    if (m_mode == 18)
    {
        m_owner->uninit();
        m_owner->stop();
        uint64_t id = 99;
        if (m_owner->nextRequestId(id) != NI_INVALID_STATE || id != 0 || m_owner->submit(request) != NI_INVALID_STATE) { return NI_PROTOCOL_ERROR; }
    }
    return NI_OK;
}

int32_t NetFileIoTestBackend::cancel(uint64_t id)
{
    (void)id;
    if (m_mode == 10) { throw 10; }
    return NI_NOT_FOUND;
}

int32_t NetFileIoTestBackend::peek(NetFileIoResult& result)
{
    std::memset(&result, 0, sizeof(result));
    if (m_throw)
    {
        result.id = 0x4343;
        result.bytes = reinterpret_cast<const uint8_t*>(1);
        throw 1;
    }
    if (m_mode == 4) { result.id = 44; result.bytes = reinterpret_cast<const uint8_t*>(1); }
    if (m_mode >= 11 && m_mode <= 15)
    {
        result.id = m_mode == 11 ? 0 : 7;
        result.operation = m_mode == 12 ? 0 : NET_FILE_IO_READ;
        result.result = m_mode == 15 ? NI_READY : NI_OK;
        result.size = m_mode == 13 ? 65537 : m_mode == 14 ? 1 : 0;
        return NI_READY;
    }
    return NI_NEED_MORE;
}

int32_t NetFileIoTestBackend::consume(uint64_t id)
{
    (void)id;
    if (m_mode == 16) { throw 16; }
    return NI_INVALID_STATE;
}

void NetFileIoTestBackend::stop()
{
    m_stopped = true;
    if (m_mode == 3) { throw 3; }
}

bool NetFileIoTestBackend::done() const
{
    if (m_mode == 17) { throw 17; }
    return m_done;
}

void NetFileIoTestBackend::uninit()
{
    if (m_mode == 20) { throw 20; }
}

NetFileIoBackend* NetFileIoTestBackend::create()
{
    if (m_mode == 1)
    {
        return nullptr;
    }
    if (m_mode == 2)
    {
        throw 1;
    }
    return new NetFileIoTestBackend();
}