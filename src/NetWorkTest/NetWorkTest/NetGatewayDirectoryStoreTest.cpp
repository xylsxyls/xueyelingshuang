#include "NetPeer/NetGatewayDirectoryStore.h"
#include "NetGatewayDirectoryStoreTest.h"
#include <cstring>
#include <cstdio>

NetDirectoryStoreTestBackend::NetDirectoryStoreTestBackend() :
m_busy(0),
m_accepted(0),
m_consumed(0),
m_ready(false),
m_job(),
m_reply()
{

}

int32_t NetDirectoryStoreTestBackend::submit(const NetFileIoRequest& request)
{
    if (m_busy != 0)
    {
        --m_busy;
        return NI_LIMIT_EXCEEDED;
    }
    if (m_job.id != 0 || m_ready)
    {
        return NI_INVALID_STATE;
    }
    m_job = request;
    m_job.bytes = nullptr;
    m_job.path = nullptr;
    m_job.destination = nullptr;
    if (request.operation == NET_FILE_IO_WRITE && request.size != 0)
    {
        m_copy.assign(request.bytes, request.bytes + request.size);
    }
    ++m_accepted;
    return NI_OK;
}

int32_t NetDirectoryStoreTestBackend::peek(NetFileIoResult& result)
{
    result = m_ready ? m_reply : NetFileIoResult();
    return m_ready ? NI_READY : NI_NEED_MORE;
}

int32_t NetDirectoryStoreTestBackend::consume(uint64_t id)
{
    if (!m_ready || id != m_reply.id)
    {
        return NI_INVALID_STATE;
    }
    m_ready = false;
    m_job = NetFileIoRequest();
    m_reply = NetFileIoResult();
    ++m_consumed;
    return NI_OK;
}

void NetDirectoryStoreTestBackend::finish(int32_t result, uint64_t file, uint64_t length, const uint8_t* bytes, uint32_t size)
{
    m_reply = NetFileIoResult();
    m_reply.id = m_job.id;
    m_reply.operation = m_job.operation;
    m_reply.result = result;
    m_reply.file = file;
    m_reply.length = length;
    m_reply.bytes = bytes;
    m_reply.size = size;
    m_ready = true;
}

NetFileIoBackend* NetDirectoryStoreTestBackend::create()
{
    return new NetDirectoryStoreTestBackend();
}