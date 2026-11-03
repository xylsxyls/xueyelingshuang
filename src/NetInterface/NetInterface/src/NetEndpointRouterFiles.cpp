#include "NetInterfaceMacro.h"
#include "NetEndpointRouterState.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include "NetEndpointRouter.h"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <utility>

NetEndpointFileStream::NetEndpointFileStream(uint64_t client, uint64_t service, uint32_t product, const NetInterfaceFileInfo& info,
                                           uint64_t deadline) :
m_id(0),
m_expiryIndex(0),
m_client(client),
m_service(service),
m_local(info.stream),
m_deadline(deadline),
m_product(product),
m_info(info)
{
    std::memset(m_previous, 0, sizeof(m_previous));
    std::memset(m_next, 0, sizeof(m_next));
}

NetEndpointFileStream::~NetEndpointFileStream()
{
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_info), sizeof(m_info));
}

NetEndpointSecretScope::NetEndpointSecretScope(void* bytes, uint32_t size) :
m_bytes(bytes), m_size(size)
{

}

NetEndpointSecretScope::~NetEndpointSecretScope()
{
    NetInterface::wipe(static_cast<uint8_t*>(m_bytes), m_size);
}

void NetEndpointRouterState::exchangeFile(uint32_t left, uint32_t right)
{
    std::swap(m_fileExpiry[left], m_fileExpiry[right]);
    m_fileExpiry[left]->m_expiryIndex = left;
    m_fileExpiry[right]->m_expiryIndex = right;
}

void NetEndpointRouterState::riseFile(uint32_t index)
{
    while (index != 0)
    {
        const uint32_t parent = (index - 1) / 2;
        if (m_fileExpiry[parent]->m_deadline <= m_fileExpiry[index]->m_deadline)
        {
            break;
        }
        exchangeFile(parent, index);
        index = parent;
    }
}

void NetEndpointRouterState::sinkFile(uint32_t index)
{
    while (index * 2 + 1 < m_fileExpiryCount)
    {
        uint32_t child = index * 2 + 1;
        if (child + 1 < m_fileExpiryCount && m_fileExpiry[child + 1]->m_deadline < m_fileExpiry[child]->m_deadline)
        {
            ++child;
        }
        if (m_fileExpiry[index]->m_deadline <= m_fileExpiry[child]->m_deadline)
        {
            break;
        }
        exchangeFile(index, child);
        index = child;
    }
}

void NetEndpointRouterState::attachFile(NetEndpointFileStream* stream)
{
    const uint64_t ids[2] = {stream->m_client, stream->m_service};
    for (uint32_t role = 0; role < 2; ++role)
    {
        NetEndpointPeer* endpoint = m_endpoints.find(ids[role])->second.get();
        stream->m_next[role] = endpoint->m_firstFile;
        if (endpoint->m_firstFile != nullptr)
        {
            endpoint->m_firstFile->m_previous[role] = stream;
        }
        endpoint->m_firstFile = stream;
        ++endpoint->m_fileCount;
    }
    stream->m_expiryIndex = m_fileExpiryCount;
    m_fileExpiry[m_fileExpiryCount++] = stream;
    riseFile(stream->m_expiryIndex);
}

std::unique_ptr<NetEndpointFileStream> NetEndpointRouterState::takeFile(uint64_t id)
{
    const auto found = m_streams.find(id);
    if (found == m_streams.end())
    {
        return std::unique_ptr<NetEndpointFileStream>();
    }
    std::unique_ptr<NetEndpointFileStream> stream = std::move(found->second);
    m_streams.erase(found);
    m_serviceStreams.erase(std::make_pair(stream->m_service, stream->m_local));
    const uint64_t ids[2] = {stream->m_client, stream->m_service};
    for (uint32_t role = 0; role < 2; ++role)
    {
        NetEndpointPeer* endpoint = m_endpoints.find(ids[role])->second.get();
        if (stream->m_previous[role] != nullptr)
        {
            stream->m_previous[role]->m_next[role] = stream->m_next[role];
        }
        else
        {
            endpoint->m_firstFile = stream->m_next[role];
        }
        if (stream->m_next[role] != nullptr)
        {
            stream->m_next[role]->m_previous[role] = stream->m_previous[role];
        }
        --endpoint->m_fileCount;
    }
    const uint32_t index = stream->m_expiryIndex;
    --m_fileExpiryCount;
    if (index != m_fileExpiryCount)
    {
        NetEndpointFileStream* replacement = m_fileExpiry[m_fileExpiryCount];
        m_fileExpiry[index] = replacement;
        replacement->m_expiryIndex = index;
        riseFile(index);
        sinkFile(replacement->m_expiryIndex);
    }
    m_fileExpiry[m_fileExpiryCount] = nullptr;
    return stream;
}

void NetEndpointRouterState::renewFile(NetEndpointFileStream* stream, uint64_t deadline)
{
    stream->m_deadline = deadline;
    riseFile(stream->m_expiryIndex);
    sinkFile(stream->m_expiryIndex);
}

int32_t NetEndpointRouter::bindFileReply(const NetInterfaceRequest& request, const NetInterfaceRpcView& rpc, std::vector<uint8_t>& payload)
{
    try
    {
        payload.resize(NET_INTERFACE_FILE_INFO_SIZE + 16);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    NetInterfaceFileInfo info = {};
    NetEndpointSecretScope infoScope(&info, sizeof(info));
    if (NetInterfaceFile::parseInfo(rpc.m_data, rpc.m_size, info) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    const auto client = m_state->m_endpoints.find(request.m_source);
    const auto service = m_state->m_endpoints.find(request.m_target);
    if (client == m_state->m_endpoints.end() || service == m_state->m_endpoints.end() || client->second->m_service ||
        !service->second->m_service ||
        (service->second->m_product != request.m_product && request.m_target != m_state->m_generic) || client->second->m_closing ||
        service->second->m_closing)
    {
        return NI_INVALID_STATE;
    }
    uint32_t negotiated[4] = {};
    uint8_t nonce[16] = {};
    uint8_t device[16] = {};
    if (client->second->m_attached)
    {
        std::memcpy(negotiated, client->second->m_negotiated, sizeof(negotiated));
    }
    else if (client->second->m_handshake->peerInfo(negotiated, nonce, device) != NI_READY)
    {
        return NI_NOT_SUPPORTED;
    }
    if ((negotiated[1] & 2) == 0 ||
        info.chunk > negotiated[3])
    {
        return NI_NOT_SUPPORTED;
    }
    if (info.state == NI_FILE_TRANSFERRING)
    {
        const int64_t utc = static_cast<int64_t>(std::time(nullptr));
        if (utc <= 0 || info.expiresUtc <= static_cast<uint64_t>(utc))
        {
            return NI_TIMEOUT;
        }
        const uint64_t deadline =
            NetEndpointRouterState::now() +
            (std::min<uint64_t>)(UINT64_C(1800000),
                (std::min<uint64_t>)(UINT64_C(1800), info.expiresUtc - static_cast<uint64_t>(utc)) * 1000);
        const auto key = std::make_pair(request.m_target, info.stream);
        const auto existing = m_state->m_serviceStreams.find(key);
        uint64_t publicId = 0;
        if (existing != m_state->m_serviceStreams.end())
        {
            const auto stream = m_state->m_streams.find(existing->second);
            if (stream == m_state->m_streams.end() || stream->second->m_client != request.m_source ||
                stream->second->m_product != request.m_product || stream->second->m_info.direction != info.direction ||
                stream->second->m_info.total != info.total || stream->second->m_info.chunk != info.chunk ||
                stream->second->m_info.window != info.window || !NetInterfaceFile::equal(stream->second->m_info.hash, info.hash, 32) ||
                !NetInterfaceFile::equal(stream->second->m_info.scope, info.scope, 16) ||
                !NetInterfaceFile::equal(stream->second->m_info.token, info.token, 32) ||
                !NetInterfaceFile::equal(stream->second->m_info.transfer, info.transfer, 16))
            {
                return NI_PROTOCOL_ERROR;
            }
            m_state->renewFile(stream->second.get(), deadline);
            publicId = existing->second;
        }
        else
        {
            if (m_state->m_streams.size() >= 1024 || client->second->m_fileCount >= 32 ||
                service->second->m_fileCount >= 128 || m_state->m_lastStream == UINT64_MAX)
            {
                return NI_LIMIT_EXCEEDED;
            }
            publicId = ++m_state->m_lastStream;
            try
            {
                std::unique_ptr<NetEndpointFileStream> stream(new NetEndpointFileStream(
                    request.m_source, request.m_target, request.m_product, info, deadline));
                stream->m_id = publicId;
                m_state->m_streams.insert(std::make_pair(publicId, std::move(stream)));
                m_state->m_serviceStreams.insert(std::make_pair(key, publicId));
                // 所有可失败分配已经完成，再提交无分配的额度、链及期限索引。
                m_state->attachFile(m_state->m_streams.find(publicId)->second.get());
            }
            catch (...)
            {
                m_state->m_streams.erase(publicId);
                m_state->m_serviceStreams.erase(key);
                return NI_NO_MEMORY;
            }
        }
        info.stream = publicId;
    }
    else
    {
        NetEndpointFileStream* stream = client->second->m_firstFile;
        while (stream != nullptr)
        {
            NetEndpointFileStream* next = stream->m_next[0];
            if (stream->m_service == request.m_target && stream->m_product == request.m_product &&
                NetInterfaceFile::equal(stream->m_info.transfer, info.transfer, 16))
            {
                dropFile(stream->m_id, info.state == NI_FILE_COMPLETED ? NI_NOT_FOUND : NI_CANCELLED);
            }
            stream = next;
        }
    }
    uint8_t data[NET_INTERFACE_FILE_INFO_SIZE] = {};
    NetEndpointSecretScope dataScope(data, sizeof(data));
    const int32_t encoded = NetInterfaceFile::encodeInfo(info, data, sizeof(data));
    if (encoded != NI_OK)
    {
        return encoded;
    }
    uint32_t required = 0;
    return NetInterfaceRpc::encode(NI_RESPONSE, NI_RPC_SUCCESS, 0, data, sizeof(data), payload.data(),
        static_cast<uint32_t>(payload.size()), required);
}

int32_t NetEndpointRouter::routeFile(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto source = m_state->m_endpoints.find(endpoint);
    if (source == m_state->m_endpoints.end() || source->second->m_closing)
    {
        return NI_NOT_FOUND;
    }
    uint64_t id = frame.m_requestId;
    if (source->second->m_service)
    {
        const auto mapped = m_state->m_serviceStreams.find(std::make_pair(endpoint, id));
        if (mapped == m_state->m_serviceStreams.end())
        {
            return NI_OK;
        }
        id = mapped->second;
    }
    const auto found = m_state->m_streams.find(id);
    if (found == m_state->m_streams.end())
    {
        return NI_OK;
    }
    NetEndpointFileStream* stream = found->second.get();
    if ((source->second->m_service ? endpoint != stream->m_service : endpoint != stream->m_client) ||
        frame.m_productId != stream->m_product)
    {
        return NI_PROTOCOL_ERROR;
    }
    if (NetEndpointRouterState::now() >= stream->m_deadline)
    {
        dropFile(id, NI_TIMEOUT);
        return NI_OK;
    }
    if (frame.m_kind == NI_CANCEL)
    {
        dropFile(id, NI_CANCELLED, endpoint);
        return NI_OK;
    }
    bool terminal = false;
    const bool sendsData = stream->m_info.direction == 1 ? !source->second->m_service : source->second->m_service;
    if (frame.m_kind == NI_DATA)
    {
        NetInterfaceFileData data = {};
        NetEndpointSecretScope dataScope(&data, sizeof(data));
        if (!sendsData || NetInterfaceFile::parseData(frame, data) != NI_OK ||
            !NetInterfaceFile::equal(data.token, stream->m_info.token, 32) || data.offset > stream->m_info.total ||
            data.size > stream->m_info.total - data.offset || data.size > stream->m_info.chunk)
        {
            return NI_PROTOCOL_ERROR;
        }
    }
    else if (frame.m_kind == NI_DATA_ACK)
    {
        NetInterfaceFileAck ack = {};
        NetEndpointSecretScope ackScope(&ack, sizeof(ack));
        if (sendsData || NetInterfaceFile::parseAck(frame, ack) != NI_OK || !NetInterfaceFile::equal(ack.token, stream->m_info.token, 32) ||
            ack.offset > stream->m_info.total)
        {
            return NI_PROTOCOL_ERROR;
        }
        terminal = ack.flags != 0;
    }
    else
    {
        NetInterfaceFileNotice notice = {};
        NetEndpointSecretScope noticeScope(&notice, sizeof(notice));
        if (!source->second->m_service || NetInterfaceFile::parseNotice(frame, notice) != NI_OK ||
            !NetInterfaceFile::equal(notice.token, stream->m_info.token, 32) ||
            !NetInterfaceFile::equal(notice.transfer, stream->m_info.transfer, 16))
        {
            return NI_PROTOCOL_ERROR;
        }
        terminal = true;
    }
    NetInterfaceFrame forwarded = frame;
    forwarded.m_requestId = source->second->m_service ? id : stream->m_local;
    const uint64_t target = source->second->m_service ? stream->m_client : stream->m_service;
    const int32_t delivered = deliver(target, forwarded, origin(stream->m_client));
    if (delivered == NI_OK && terminal)
    {
        m_state->takeFile(id);
    }
    return delivered;
}

void NetEndpointRouter::dropFile(uint64_t id, int32_t reason, uint64_t skip)
{
    std::unique_ptr<NetEndpointFileStream> stream = m_state->takeFile(id);
    if (!stream)
    {
        return;
    }
    NetInterfaceFileNotice notice = {};
    NetEndpointSecretScope noticeScope(&notice, sizeof(notice));
    notice.result = reason < 0 ? reason : NI_CANCELLED;
    std::memcpy(notice.token, stream->m_info.token, 32);
    std::memcpy(notice.transfer, stream->m_info.transfer, 16);
    uint8_t payload[NET_INTERFACE_FILE_NOTICE_SIZE] = {};
    NetEndpointSecretScope payloadScope(payload, sizeof(payload));
    if (NetInterfaceFile::encodeNotice(notice, payload, sizeof(payload)) != NI_OK)
    {
        return;
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_EVENT;
    frame.m_productId = stream->m_product;
    frame.m_messageType = NI_FILE_CANCEL;
    frame.m_payload = payload;
    frame.m_payloadSize = sizeof(payload);
    const auto context = origin(stream->m_client);
    frame.m_requestId = id;
    if (skip != stream->m_client && deliver(stream->m_client, frame, context) != NI_OK)
    {
        closeEndpoint(stream->m_client, NI_LIMIT_EXCEEDED);
    }
    frame.m_requestId = stream->m_local;
    if (skip != stream->m_service && deliver(stream->m_service, frame, context) != NI_OK)
    {
        closeEndpoint(stream->m_service, NI_LIMIT_EXCEEDED);
    }
}

void NetEndpointRouter::collectFiles(uint64_t endpoint)
{
    if (endpoint != 0)
    {
        const auto found = m_state->m_endpoints.find(endpoint);
        if (found == m_state->m_endpoints.end())
        {
            return;
        }
        while (found->second->m_firstFile != nullptr)
        {
            dropFile(found->second->m_firstFile->m_id, NI_INVALID_STATE, endpoint);
        }
        return;
    }
    const uint64_t now = NetEndpointRouterState::now();
    for (uint32_t count = 0; count < 64 && m_state->m_fileExpiryCount != 0; ++count)
    {
        NetEndpointFileStream* stream = m_state->m_fileExpiry[0];
        if (now < stream->m_deadline)
        {
            break;
        }
        dropFile(stream->m_id, NI_TIMEOUT);
    }
}

int32_t NetEndpointRouter::routeEvent(uint64_t endpoint, const NetInterfaceFrame& frame)
{
    const auto source = m_state->m_endpoints.find(endpoint);
    if (source == m_state->m_endpoints.end() || !source->second->m_service ||
        (source->second->m_product != 0 && frame.m_productId != source->second->m_product))
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceEventView event = {};
    if (NetInterfaceEvent::parse(frame, event) != NI_OK)
    {
        return NI_PROTOCOL_ERROR;
    }
    uint64_t targetEndpoint = event.m_target;
    const auto publicTarget = m_state->m_origins.find(event.m_target);
    if (publicTarget != m_state->m_origins.end())
    {
        targetEndpoint = publicTarget->second;
    }
    else if (m_state->m_clients != nullptr && event.m_target != 0)
    {
        return NI_OK;
    }
    if (event.m_flags == 1)
    {
        for (auto item = m_state->m_streams.begin(); item != m_state->m_streams.end();)
        {
            const uint64_t id = item->first;
            const auto current = item++;
            if ((event.m_target == 0 || current->second->m_client == targetEndpoint) &&
                (source->second->m_product == 0 || current->second->m_service == endpoint) &&
                NetInterfaceFile::equal(current->second->m_info.scope, event.m_data, 16))
            {
                dropFile(id, NI_CANCELLED);
            }
        }
        return NI_OK;
    }
    const auto target = m_state->m_endpoints.find(targetEndpoint);
    if (target == m_state->m_endpoints.end() || target->second->m_service || target->second->m_closing)
    {
        return NI_OK;
    }
    if (m_state->m_lastEvent == UINT64_MAX)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetInterfaceFrame forwarded = frame;
    forwarded.m_requestId = m_state->m_lastEvent + 1;
    forwarded.m_payload = event.m_data;
    forwarded.m_payloadSize = event.m_size;
    const int32_t result = deliver(targetEndpoint, forwarded, origin(targetEndpoint));
    if (result == NI_OK)
    {
        ++m_state->m_lastEvent;
    }
    return result;
}