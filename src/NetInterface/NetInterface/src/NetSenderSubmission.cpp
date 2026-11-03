#include "NetSenderSubmission.h"
#include "NetInterfaceMacro.h"
#include "NetSenderSubmissionState.h"
#include "NetInterface.h"
#include <chrono>
#include <cstring>
#include <new>

NetSenderSubmission::NetSenderSubmission() :
m_state(new NetSenderSubmissionState())
{

}

NetSenderSubmission::~NetSenderSubmission()
{
    delete m_state;
}

int32_t NetSenderSubmission::configureGate(const NetSenderSubmissionGate& gate)
{
    if ((gate.m_enter == nullptr) != (gate.m_leave == nullptr))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_gateConfigured || m_state->m_generation != 0)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_gate = gate;
    m_state->m_gateConfigured = true;
    return NI_OK;
}

int32_t NetSenderSubmission::start(uint64_t generation, uint32_t product)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (generation == 0 || m_state->m_requests != nullptr || m_state->m_pending != nullptr ||
        m_state->m_takenKind != NET_SUBMIT_NONE || m_state->m_receipts != nullptr)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_generation = generation;
    m_state->m_product = product;
    m_state->m_ready = false;
    m_state->m_closing = false;
    m_state->m_closeTaken = false;
    m_state->m_reason = 0;
    return NI_OK;
}

int32_t NetSenderSubmission::ready(uint64_t generation, uint32_t requests, uint32_t chunk)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (generation == 0 || generation < m_state->m_generation || requests == 0 || requests > NetSenderSubmissionState::kRequests ||
        chunk < 1024 || chunk > 65536 || (generation != m_state->m_generation &&
        (m_state->m_requests != nullptr || m_state->m_pending != nullptr || m_state->m_takenKind != NET_SUBMIT_NONE || m_state->pendingReceipts())))
    {
        return NI_PROTOCOL_ERROR;
    }
    if (m_state->m_closing)
    {
        return NI_CANCELLED;
    }
    m_state->m_generation = generation;
    m_state->m_limit = requests;
    m_state->m_chunk = chunk;
    m_state->m_ready = true;
    return NI_OK;
}

uint64_t NetSenderSubmission::now()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

int32_t NetSenderSubmission::begin(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& id)
{
    id = 0;
    const uint64_t started = now();
    if (product == 0 || timeoutMs == 0 || timeoutMs > 60000 || started > UINT64_MAX - timeoutMs)
    {
        return NI_INVALID_ARGUMENT;
    }
    {
        NetSenderSubmissionGuard lock(m_state->m_gate);
        if (!m_state->m_ready || m_state->m_closing)
        {
            return NI_INVALID_STATE;
        }
        if (m_state->m_product != 0 && product != m_state->m_product)
        {
            return NI_INVALID_ARGUMENT;
        }
        if (m_state->m_requestCount >= m_state->m_limit || m_state->m_nextId == UINT64_MAX)
        {
            return NI_LIMIT_EXCEEDED;
        }
        NetSenderSubmissionEntry* entry = new (std::nothrow) NetSenderSubmissionEntry();
        if (entry == nullptr)
        {
            return NI_NO_MEMORY;
        }
        entry->m_command.m_kind = NET_SUBMIT_BEGIN;
        entry->m_command.m_generation = m_state->m_generation;
        entry->m_command.m_id = ++m_state->m_nextId;
        entry->m_command.m_product = product;
        entry->m_command.m_type = type;
        entry->m_command.m_started = started;
        entry->m_command.m_timeout = timeoutMs;
        entry->m_previousRequest = m_state->m_requestsTail;
        if (m_state->m_requestsTail != nullptr)
        {
            m_state->m_requestsTail->m_nextRequest = entry;
        }
        else
        {
            m_state->m_requests = entry;
        }
        m_state->m_requestsTail = entry;
        ++m_state->m_requestCount;
        id = entry->m_command.m_id;
    }
    if (m_state->m_gate.m_wake != nullptr)
    {
        m_state->m_gate.m_wake(m_state->m_gate.m_context);
    }
    return NI_OK;
}

int32_t NetSenderSubmission::send(uint64_t id, const uint8_t* bytes, uint32_t size)
{
    uint64_t unused = 0;
    return submitRequest(id, bytes, size, false, unused);
}

int32_t NetSenderSubmission::sendSynchronous(uint64_t id, const uint8_t* bytes, uint32_t size, uint64_t& ticket)
{
    return submitRequest(id, bytes, size, true, ticket);
}

int32_t NetSenderSubmission::submitRequest(uint64_t id, const uint8_t* bytes, uint32_t size, bool synchronous, uint64_t& ticket)
{
    ticket = 0;
    if (size > 1048560 || (size != 0 && (bytes == nullptr || reinterpret_cast<uintptr_t>(bytes) > UINTPTR_MAX - size)))
    {
        return NI_INVALID_ARGUMENT;
    }
    {
        NetSenderSubmissionGuard lock(m_state->m_gate);
        if (!m_state->m_ready || m_state->m_closing)
        {
            return NI_INVALID_STATE;
        }
        NetSenderSubmissionEntry* entry = m_state->find(id);
        if (entry == nullptr || entry->m_cancelled)
        {
            return NI_NOT_FOUND;
        }
        if (entry->m_phase != NET_SUBMISSION_RESERVED)
        {
            return NI_INVALID_STATE;
        }
        const uint64_t deadline = entry->m_command.m_started + entry->m_command.m_timeout;
        if (now() >= deadline)
        {
            return NI_TIMEOUT;
        }
        if (size > NetSenderSubmissionState::kInputBytes - m_state->m_bytes ||
            (synchronous && (m_state->m_receiptCount >= 128 || m_state->m_nextSync == UINT64_MAX)))
        {
            return NI_LIMIT_EXCEEDED;
        }
        NetSenderSubmissionReceipt* receipt = synchronous ? m_state->createReceipt(m_state->m_generation, deadline) : nullptr;
        if (synchronous && receipt == nullptr)
        {
            return NI_NO_MEMORY;
        }
        if (!entry->copy(bytes, size))
        {
            delete receipt;
            return NI_NO_MEMORY;
        }
        entry->m_phase = NET_SUBMISSION_QUEUED;
        m_state->m_bytes += size;
        m_state->append(entry);
        if (receipt != nullptr)
        {
            m_state->addReceipt(receipt, entry);
            ticket = receipt->m_ticket;
        }
    }
    if (m_state->m_gate.m_wake != nullptr)
    {
        m_state->m_gate.m_wake(m_state->m_gate.m_context);
    }
    return NI_OK;
}

int32_t NetSenderSubmission::cancel(uint64_t id)
{
    {
        NetSenderSubmissionGuard lock(m_state->m_gate);
        NetSenderSubmissionEntry* entry = m_state->find(id);
        if (entry == nullptr || entry->m_cancelled)
        {
            return NI_NOT_FOUND;
        }
        entry->m_cancelled = true;
    }
    if (m_state->m_gate.m_wake != nullptr)
    {
        m_state->m_gate.m_wake(m_state->m_gate.m_context);
    }
    return NI_OK;
}

int32_t NetSenderSubmission::data(const NetInterfaceFrame& frame)
{
    uint64_t unused = 0;
    return submitData(frame, false, unused);
}

int32_t NetSenderSubmission::dataSynchronous(const NetInterfaceFrame& frame, uint64_t& ticket)
{
    return submitData(frame, true, ticket);
}

int32_t NetSenderSubmission::submitData(const NetInterfaceFrame& frame, bool synchronous, uint64_t& ticket)
{
    ticket = 0;
    if ((frame.m_kind != NI_DATA && frame.m_kind != NI_DATA_ACK &&
        !(frame.m_kind == NI_CANCEL && frame.m_messageType == NI_FILE_DATA_TYPE && frame.m_payloadSize == 0)) ||
        frame.m_productId == 0 || frame.m_requestId == 0 || (frame.m_payloadSize != 0 &&
        (frame.m_payload == nullptr || reinterpret_cast<uintptr_t>(frame.m_payload) > UINTPTR_MAX - frame.m_payloadSize)))
    {
        return NI_INVALID_ARGUMENT;
    }
    {
        NetSenderSubmissionGuard lock(m_state->m_gate);
        if (!m_state->m_ready || m_state->m_closing)
        {
            return NI_INVALID_STATE;
        }
        if (frame.m_payloadSize > m_state->m_chunk + 128 ||
            (m_state->m_product != 0 && frame.m_productId != m_state->m_product))
        {
            return NI_INVALID_ARGUMENT;
        }
        const bool control = frame.m_kind != NI_DATA;
        uint32_t& count = control ? m_state->m_controlCount : m_state->m_frameCount;
        uint32_t& used = control ? m_state->m_controlBytes : m_state->m_bytes;
        const uint32_t slots = control ? NetSenderSubmissionState::kControls : NetSenderSubmissionState::kFrames;
        const uint32_t limit = control ? NetSenderSubmissionState::kControlBytes : NetSenderSubmissionState::kInputBytes;
        if (count >= slots || frame.m_payloadSize > limit - used || m_state->m_nextFrame == UINT64_MAX ||
            (synchronous && (m_state->m_receiptCount >= 128 || m_state->m_nextSync == UINT64_MAX)))
        {
            return NI_LIMIT_EXCEEDED;
        }
        const uint64_t current = now();
        if (synchronous && current > UINT64_MAX - 5000)
        {
            return NI_INVALID_STATE;
        }
        NetSenderSubmissionEntry* entry = new (std::nothrow) NetSenderSubmissionEntry();
        if (entry == nullptr)
        {
            return NI_NO_MEMORY;
        }
        if (!entry->copy(frame.m_payload, frame.m_payloadSize))
        {
            delete entry;
            return NI_NO_MEMORY;
        }
        NetSenderSubmissionReceipt* receipt = synchronous ? m_state->createReceipt(m_state->m_generation, current + 5000) : nullptr;
        if (synchronous && receipt == nullptr)
        {
            delete entry;
            return NI_NO_MEMORY;
        }
        entry->m_phase = NET_SUBMISSION_QUEUED;
        entry->m_command.m_kind = NET_SUBMIT_DATA;
        entry->m_command.m_generation = m_state->m_generation;
        entry->m_command.m_id = ++m_state->m_nextFrame;
        entry->m_command.m_frame = frame;
        entry->m_command.m_frame.m_payload = entry->m_input;
        ++count;
        used += frame.m_payloadSize;
        m_state->append(entry);
        if (receipt != nullptr)
        {
            m_state->addReceipt(receipt, entry);
            ticket = receipt->m_ticket;
        }
    }
    if (m_state->m_gate.m_wake != nullptr)
    {
        m_state->m_gate.m_wake(m_state->m_gate.m_context);
    }
    return NI_OK;
}

void NetSenderSubmission::close(int32_t reason)
{
    {
        NetSenderSubmissionGuard lock(m_state->m_gate);
        m_state->m_ready = false;
        if (!m_state->m_closing)
        {
            m_state->m_closing = true;
            m_state->m_reason = reason;
        }
    }
    if (m_state->m_gate.m_wake != nullptr)
    {
        m_state->m_gate.m_wake(m_state->m_gate.m_context);
    }
}

bool NetSenderSubmission::closeRequested(int32_t& reason) const
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    reason = m_state->m_closing ? m_state->m_reason : 0;
    return m_state->m_closing;
}

void NetSenderSubmission::state(uint64_t& generation, bool& ready) const
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    generation = m_state->m_generation;
    ready = m_state->m_ready && !m_state->m_closing;
}

int32_t NetSenderSubmission::beginPoll()
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return NI_INVALID_STATE;
    }
    if (++m_state->m_round == 0)
    {
        m_state->m_round = 1;
        for (NetSenderSubmissionEntry* entry = m_state->m_pending; entry != nullptr; entry = entry->m_nextPending)
        {
            entry->m_attempt = 0;
        }
    }
    return NI_OK;
}

int32_t NetSenderSubmission::next(NetSenderSubmissionCommand& command)
{
    std::memset(&command, 0, sizeof(command));
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_closing)
    {
        if (m_state->m_closeTaken)
        {
            return NI_NEED_MORE;
        }
        m_state->m_closeTaken = true;
        m_state->m_takenKind = NET_SUBMIT_CLOSE;
        command.m_kind = NET_SUBMIT_CLOSE;
        command.m_generation = m_state->m_generation;
        command.m_status = m_state->m_reason;
        return NI_READY;
    }
    if (!m_state->m_ready)
    {
        return NI_NEED_MORE;
    }
    const uint64_t current = now();
    m_state->expireFrames(current);
    for (NetSenderSubmissionEntry* entry = m_state->m_requests; entry != nullptr; entry = entry->m_nextRequest)
    {
        const bool registered = entry->m_phase == NET_SUBMISSION_REGISTERED || entry->m_phase == NET_SUBMISSION_SENT;
        if (!entry->m_cancelTaken && (entry->m_cancelled ||
            (!registered && current >= entry->m_command.m_started + entry->m_command.m_timeout)))
        {
            entry->m_cancelTaken = true;
            command = entry->m_command;
            command.m_kind = registered ? NET_SUBMIT_CANCEL : NET_SUBMIT_TERMINAL;
            command.m_status = entry->m_cancelled ? NI_RPC_CANCELLED : NI_RPC_TIMEOUT;
            m_state->m_taken = entry;
            m_state->m_takenKind = command.m_kind;
            return NI_READY;
        }
    }
    NetSenderSubmissionEntry* entry = m_state->m_pending;
    if (m_state->m_draining != nullptr || entry == nullptr || entry->m_cancelTaken)
    {
        return NI_NEED_MORE;
    }
    const bool frame = entry->m_command.m_kind == NET_SUBMIT_DATA;
    const bool send = frame || entry->m_phase == NET_SUBMISSION_REGISTERED;
    if (send && entry->m_attempt == m_state->m_round)
    {
        return NI_NEED_MORE;
    }
    command = entry->m_command;
    command.m_kind = frame ? NET_SUBMIT_DATA : send ? NET_SUBMIT_SEND : NET_SUBMIT_BEGIN;
    if (send)
    {
        entry->m_attempt = m_state->m_round;
        if (!frame)
        {
            command.m_bytes = entry->m_input;
            command.m_size = entry->m_size;
        }
    }
    m_state->m_taken = entry;
    m_state->m_takenKind = command.m_kind;
    return NI_READY;
}

void NetSenderSubmission::finish(int32_t result)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    NetSenderSubmissionEntry* entry = m_state->m_taken;
    const int32_t kind = m_state->m_takenKind;
    if (entry != nullptr)
    {
        NetSenderSubmissionReceipt* receipt = entry->m_receipt;
        if (receipt != nullptr && receipt->m_result == NI_READY)
        {
            if (kind == NET_SUBMIT_SEND || kind == NET_SUBMIT_DATA)
            {
                if (result == NI_OK)
                {
                    receipt->m_staged = true;
                    m_state->m_draining = receipt;
                }
                else if (result != NI_LIMIT_EXCEEDED)
                {
                    m_state->settle(receipt, result);
                }
            }
            else if (kind == NET_SUBMIT_BEGIN && result < 0)
            {
                m_state->settle(receipt, result == NI_LIMIT_EXCEEDED ? NI_INVALID_STATE : result);
            }
            else if (kind == NET_SUBMIT_CANCEL || kind == NET_SUBMIT_TERMINAL)
            {
                const bool timeout = !entry->m_cancelled && kind == NET_SUBMIT_TERMINAL;
                m_state->settle(receipt, timeout ? NI_TIMEOUT : NI_CANCELLED);
            }
        }
        if (entry->m_phase != NET_SUBMISSION_RETIRED && result == NI_OK)
        {
            if (kind == NET_SUBMIT_BEGIN)
            {
                entry->m_phase = NET_SUBMISSION_REGISTERED;
            }
            else if (kind == NET_SUBMIT_SEND)
            {
                entry->m_phase = NET_SUBMISSION_SENT;
                m_state->remove(entry);
                m_state->clear(entry);
            }
            else if (kind == NET_SUBMIT_CANCEL)
            {
                // 已取消请求只等协议终态，不再让其未发正文堵住后续FIFO。
                m_state->remove(entry);
                m_state->clear(entry);
            }
            else if (kind == NET_SUBMIT_DATA)
            {
                m_state->retire(entry);
            }
        }
        m_state->m_taken = nullptr;
        if (entry->m_phase == NET_SUBMISSION_RETIRED)
        {
            m_state->retire(entry);
        }
    }
    m_state->m_takenKind = NET_SUBMIT_NONE;
}

bool NetSenderSubmission::complete(uint64_t generation, uint64_t id, NetSenderSubmissionCommand& command)
{
    std::memset(&command, 0, sizeof(command));
    NetSenderSubmissionGuard lock(m_state->m_gate);
    NetSenderSubmissionEntry* entry = m_state->find(id);
    if (entry == nullptr || entry->m_command.m_generation != generation)
    {
        return false;
    }
    m_state->take(entry, command);
    return true;
}

void NetSenderSubmission::disconnected(uint64_t generation)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (generation < m_state->m_generation)
    {
        return;
    }
    m_state->m_generation = generation;
    m_state->m_ready = false;
    for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        m_state->settle(receipt, NI_INVALID_STATE);
    }
    m_state->dropFrames();
}

bool NetSenderSubmission::takeClosed(NetSenderSubmissionCommand& command)
{
    std::memset(&command, 0, sizeof(command));
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (m_state->m_requests == nullptr)
    {
        return false;
    }
    m_state->take(m_state->m_requests, command);
    return true;
}

bool NetSenderSubmission::idle() const
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    return m_state->m_requests == nullptr && m_state->m_pending == nullptr && m_state->m_receipts == nullptr &&
        m_state->m_takenKind == NET_SUBMIT_NONE;
}

uint32_t NetSenderSubmission::queuedBytes() const
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    return m_state->m_bytes;
}

bool NetSenderSubmission::synchronousPending() const
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    return m_state->pendingReceipts();
}

void NetSenderSubmission::advanceSynchronous(int32_t outputState)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    if (m_state->m_takenKind != NET_SUBMIT_NONE)
    {
        return;
    }
    const uint64_t current = now();
    if (m_state->m_ready && !m_state->m_closing)
    {
        m_state->expireFrames(current);
    }
    for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        if (receipt->m_result != NI_READY)
        {
            continue;
        }
        int32_t result = NI_READY;
        if (m_state->m_closing || !m_state->m_ready || receipt->m_generation != m_state->m_generation)
        {
            result = m_state->m_reason < 0 ? m_state->m_reason : NI_CANCELLED;
        }
        else if (current >= receipt->m_deadline)
        {
            result = NI_TIMEOUT;
        }
        else if (receipt->m_staged)
        {
            // 只有明确OK才证明交付；其它正数也仍是未完成。
            result = outputState <= NI_OK ? outputState : NI_READY;
        }
        if (result != NI_READY)
        {
            m_state->settle(receipt, result);
            if (receipt->m_staged && result < 0 && !m_state->m_closing)
            {
                // 前缀可能已发送；停止本代并丢弃后缀，不把旧正文带进重连。
                m_state->m_ready = false;
                m_state->m_closing = true;
                m_state->m_reason = result;
            }
        }
    }
    if (m_state->m_closing)
    {
        const int32_t reason = m_state->m_reason < 0 ? m_state->m_reason : NI_CANCELLED;
        for (NetSenderSubmissionReceipt* receipt = m_state->m_receipts; receipt != nullptr; receipt = receipt->m_next)
        {
            m_state->settle(receipt, reason);
        }
    }
}

int32_t NetSenderSubmission::synchronousResult(uint64_t ticket, bool consume)
{
    NetSenderSubmissionGuard lock(m_state->m_gate);
    NetSenderSubmissionReceipt** link = &m_state->m_receipts;
    while (*link != nullptr && (*link)->m_ticket != ticket)
    {
        link = &(*link)->m_next;
    }
    if (*link == nullptr)
    {
        return NI_NOT_FOUND;
    }
    NetSenderSubmissionReceipt* receipt = *link;
    const int32_t result = receipt->m_result;
    if (consume && result != NI_READY)
    {
        *link = receipt->m_next;
        if (receipt->m_entry != nullptr)
        {
            receipt->m_entry->m_receipt = nullptr;
        }
        --m_state->m_receiptCount;
        delete receipt;
    }
    return result;
}

// NET_SENDER_SUBMISSION_UNIT_BEGIN
// #include <cstdio>
//  #include <vector>
//
// #include <chrono>
// #if defined(_MSC_VER) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// int main(int argc, const char* argv[])
// {
//     if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
//     {
//         NetSenderSubmission queue;
//         if (queue.start(1, 101) != NI_OK || queue.ready(1, 128, 4096) != NI_OK)
//         {
//             return 1;
//         }
//         uint8_t bytes[128] = {};
//         std::memset(bytes, 0x6a, sizeof(bytes));
//         NetInterfaceFrame frame = {};
//         frame.m_kind = NI_DATA;
//         frame.m_productId = 101;
//         frame.m_requestId = 1;
//         frame.m_payload = bytes;
//         frame.m_payloadSize = sizeof(bytes);
//         NetSenderSubmissionCommand command = {}, metadata = {};
//         const uint32_t rounds = 20000;
//         uint64_t received = 0;
//         const auto started = std::chrono::steady_clock::now();
//         for (uint32_t round = 0; round < rounds; ++round)
//         {
//             uint64_t ids[8] = {};
//             for (uint32_t index = 0; index < 8; ++index)
//             {
//                 if (queue.begin(101, 77, 5000, ids[index]) != NI_OK ||
//                     queue.send(ids[index], bytes, sizeof(bytes)) != NI_OK || queue.data(frame) != NI_OK)
//                 {
//                     return 2;
//                 }
//             }
//             if (queue.beginPoll() != NI_OK)
//             {
//                 return 3;
//             }
//             for (uint32_t index = 0; index < 8; ++index)
//             {
//                 if (queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_BEGIN || command.m_id != ids[index])
//                 {
//                     return 4;
//                 }
//                 queue.finish(NI_OK);
//                 if (queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_SEND || command.m_id != ids[index] ||
//                     command.m_size != sizeof(bytes) || std::memcmp(command.m_bytes, bytes, sizeof(bytes)) != 0)
//                 {
//                     return 5;
//                 }
//                 queue.finish(NI_OK);
//                 if (!queue.complete(1, ids[index], metadata) || queue.next(command) != NI_READY || command.m_kind != NET_SUBMIT_DATA ||
//                     command.m_frame.m_payloadSize != sizeof(bytes) || std::memcmp(command.m_frame.m_payload, bytes, sizeof(bytes)) != 0)
//                 {
//                     return 6;
//                 }
//                 queue.finish(NI_OK);
//                 received += 2;
//             }
//             if (!queue.idle() || queue.queuedBytes() != 0)
//             {
//                 return 7;
//             }
//         }
//         const int64_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
//         if (received != static_cast<uint64_t>(rounds) * 16)
//         {
//             return 8;
//         }
//         std::printf("SUBMISSION_BENCHMARK messages=%llu microseconds=%lld PASS\n",
//             static_cast<unsigned long long>(received), static_cast<long long>(elapsed));
//         return 0;
//     }
//     uint32_t checks = 0;
//  #define SUBMIT_CHECK(value) { ++checks; if (!(value)) { std::printf("FAIL line=%u %s\n", static_cast<uint32_t>(__LINE__), #value); return 1; } }
//      NetSenderSubmission queue;
//      uint64_t low = 0, high = 0, cancelled = 0;
//      SUBMIT_CHECK(queue.start(1, 101) == NI_OK && queue.ready(1, 3, 65536) == NI_OK);
//      SUBMIT_CHECK(queue.begin(0, 700, 1000, low) == NI_INVALID_ARGUMENT && low == 0);
//      SUBMIT_CHECK(queue.begin(101, 700, 10000, low) == NI_OK);
//      SUBMIT_CHECK(queue.begin(101, 700, 10000, high) == NI_OK && high > low);
//      SUBMIT_CHECK(queue.begin(101, 700, 10000, cancelled) == NI_OK);
//      uint64_t rejected = 1;
//      SUBMIT_CHECK(queue.begin(101, 700, 1000, rejected) == NI_LIMIT_EXCEEDED && rejected == 0);
//      uint8_t byte = 7;
//      SUBMIT_CHECK(queue.send(high, &byte, 1) == NI_OK);
//      NetInterfaceFrame frame = {};
//      frame.m_kind = NI_DATA; frame.m_productId = 101; frame.m_requestId = 7001; frame.m_payload = &byte; frame.m_payloadSize = 1;
//      SUBMIT_CHECK(queue.data(frame) == NI_OK);
//      SUBMIT_CHECK(queue.send(low, &byte, 1) == NI_OK && queue.send(low, &byte, 1) == NI_INVALID_STATE);
//      byte = 99;
//      SUBMIT_CHECK(queue.cancel(cancelled) == NI_OK && queue.cancel(cancelled) == NI_NOT_FOUND);
//      NetSenderSubmissionCommand command = {}, terminal = {};
//      SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_TERMINAL && command.m_id == cancelled);
//      SUBMIT_CHECK(queue.complete(1, cancelled, terminal)); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == high); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == high && command.m_bytes[0] == 7);
//      queue.finish(NI_LIMIT_EXCEEDED);
//      SUBMIT_CHECK(queue.next(command) == NI_NEED_MORE);
//      SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_id == high);
//      queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA && command.m_frame.m_payload[0] == 7); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == low); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND && command.m_id == low && command.m_bytes[0] == 7);
//      SUBMIT_CHECK(queue.complete(1, low, terminal) && command.m_bytes[0] == 7); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.complete(1, high, terminal) && queue.idle() && queue.queuedBytes() == 0);
//      for (uint32_t index = 0; index < 64; ++index) { SUBMIT_CHECK(queue.data(frame) == NI_OK); }
//      SUBMIT_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
//      frame.m_kind = NI_DATA_ACK;
//      for (uint32_t index = 0; index < 16; ++index) { SUBMIT_CHECK(queue.data(frame) == NI_OK); }
//      SUBMIT_CHECK(queue.data(frame) == NI_LIMIT_EXCEEDED);
//      queue.close(0); queue.disconnected(1);
//      SUBMIT_CHECK(queue.idle() && queue.queuedBytes() == 0);
//      SUBMIT_CHECK(queue.start(2, 101) == NI_OK && queue.ready(2, 3, 65536) == NI_OK);
//      SUBMIT_CHECK(queue.begin(101, 700, 10000, low) == NI_OK);
//      uint64_t ticket = 0;
//      SUBMIT_CHECK(queue.sendSynchronous(low, &byte, 1, ticket) == NI_OK);
//      SUBMIT_CHECK(queue.beginPoll() == NI_OK && queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN); queue.finish(NI_OK);
//      SUBMIT_CHECK(queue.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND); queue.finish(NI_OK);
//      queue.advanceSynchronous(NI_READY);
//      SUBMIT_CHECK(queue.synchronousResult(ticket, false) == NI_READY && queue.next(command) == NI_NEED_MORE);
//      queue.advanceSynchronous(NI_OK);
//      SUBMIT_CHECK(queue.synchronousResult(ticket, true) == NI_OK && queue.complete(2, low, terminal) && queue.idle());
//      NetSenderSubmission bytes;
//      SUBMIT_CHECK(bytes.start(1, 101) == NI_OK && bytes.ready(1, 4, 65536) == NI_OK);
//      std::vector<uint8_t> big(1048560, 7);
//      uint64_t ids[3] = {};
//      for (uint32_t index = 0; index < 3; ++index) { SUBMIT_CHECK(bytes.begin(101, 700, 10000, ids[index]) == NI_OK); }
//      SUBMIT_CHECK(bytes.send(ids[0], big.data(), static_cast<uint32_t>(big.size())) == NI_OK);
//      SUBMIT_CHECK(bytes.send(ids[1], big.data(), static_cast<uint32_t>(big.size())) == NI_OK);
//      uint8_t small[64] = {};
//      SUBMIT_CHECK(bytes.send(ids[2], small, sizeof(small)) == NI_LIMIT_EXCEEDED);
//      SUBMIT_CHECK(bytes.cancel(ids[0]) == NI_OK && bytes.beginPoll() == NI_OK);
//      SUBMIT_CHECK(bytes.next(command) == NI_READY && command.m_kind == NET_SUBMIT_TERMINAL && bytes.complete(1, ids[0], terminal)); bytes.finish(NI_OK);
//      SUBMIT_CHECK(bytes.send(ids[2], small, sizeof(small)) == NI_OK);
//      bytes.close(0); bytes.disconnected(1);
//      while (bytes.takeClosed(terminal)) {}
//      SUBMIT_CHECK(bytes.idle() && bytes.queuedBytes() == 0);
//
//     NetSenderSubmission receipts;
//     SUBMIT_CHECK(receipts.start(1, 101) == NI_OK && receipts.ready(1, 128, 65536) == NI_OK);
//     uint64_t saved[128] = {};
//     frame.m_kind = NI_DATA;
//     for (uint32_t index = 0; index < 128; ++index)
//     {
//         SUBMIT_CHECK(receipts.dataSynchronous(frame, saved[index]) == NI_OK);
//         SUBMIT_CHECK(receipts.beginPoll() == NI_OK && receipts.next(command) == NI_READY);
//         receipts.finish(NI_OK);
//         receipts.advanceSynchronous(NI_OK);
//         SUBMIT_CHECK(receipts.synchronousResult(saved[index], false) == NI_OK && receipts.queuedBytes() == 0);
//     }
//     SUBMIT_CHECK(receipts.dataSynchronous(frame, ticket) == NI_LIMIT_EXCEEDED && ticket == 0);
//     SUBMIT_CHECK(receipts.start(2, 101) == NI_INVALID_STATE);
//     // 自动恢复允许保留已完成回执；慢等待者不能阻止其他生产者继续连接。
//     SUBMIT_CHECK(receipts.ready(2, 128, 65536) == NI_OK);
//     SUBMIT_CHECK(receipts.synchronousResult(saved[127], false) == NI_OK);
//     for (uint32_t index = 128; index != 0; --index)
//     {
//         SUBMIT_CHECK(receipts.synchronousResult(saved[index - 1], true) == NI_OK);
//         SUBMIT_CHECK(receipts.synchronousResult(saved[index - 1], true) == NI_NOT_FOUND);
//     }
//     SUBMIT_CHECK(receipts.idle() && receipts.dataSynchronous(frame, ticket) == NI_OK && ticket > saved[127]);
//     SUBMIT_CHECK(receipts.ready(3, 128, 65536) == NI_PROTOCOL_ERROR);
//     SUBMIT_CHECK(receipts.beginPoll() == NI_OK && receipts.next(command) == NI_READY);
//     receipts.disconnected(2);
//     SUBMIT_CHECK(command.m_frame.m_payloadSize == 1 && command.m_frame.m_payload[0] == 99);
//     SUBMIT_CHECK(receipts.synchronousResult(ticket, true) == NI_INVALID_STATE);
//     receipts.finish(NI_OK);
//     SUBMIT_CHECK(receipts.idle() && receipts.queuedBytes() == 0);
//     SUBMIT_CHECK(receipts.next(command) == NI_NEED_MORE);
//
//     NetSenderSubmission cancellation;
//     SUBMIT_CHECK(cancellation.start(1, 101) == NI_OK && cancellation.ready(1, 128, 4096) == NI_OK);
//     SUBMIT_CHECK(cancellation.begin(101, 0, 5000, low) == NI_OK);
//     SUBMIT_CHECK(cancellation.send(low, &byte, 1) == NI_OK && cancellation.data(frame) == NI_OK);
//     SUBMIT_CHECK(cancellation.beginPoll() == NI_OK && cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN);
//     cancellation.finish(NI_OK);
//     SUBMIT_CHECK(cancellation.cancel(low) == NI_OK);
//     SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_CANCEL);
//     cancellation.finish(NI_OK);
//     SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
//     cancellation.finish(NI_OK);
//     SUBMIT_CHECK(cancellation.complete(1, low, terminal) && cancellation.idle());
//     SUBMIT_CHECK(cancellation.begin(101, 0, 5000, low) == NI_OK);
//     const uint8_t* wrapped = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 3);
//     SUBMIT_CHECK(cancellation.send(low, wrapped, 8) == NI_INVALID_ARGUMENT);
//     frame.m_payload = wrapped;
//     frame.m_payloadSize = 8;
//     SUBMIT_CHECK(cancellation.data(frame) == NI_INVALID_ARGUMENT);
//     SUBMIT_CHECK(cancellation.sendSynchronous(low, &byte, 1, ticket) == NI_OK);
//     SUBMIT_CHECK(cancellation.beginPoll() == NI_OK && cancellation.next(command) == NI_READY);
//     cancellation.finish(NI_OK);
//     SUBMIT_CHECK(cancellation.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
//     cancellation.finish(NI_OK);
//     cancellation.advanceSynchronous(NI_NEED_MORE);
//     SUBMIT_CHECK(cancellation.synchronousResult(ticket, false) == NI_READY);
//     cancellation.advanceSynchronous(NI_OK);
//     SUBMIT_CHECK(cancellation.synchronousResult(ticket, true) == NI_OK && cancellation.complete(1, low, terminal));
//
//     // 反向预留正文，与DATA和ACK混排；独立预期只取成功接纳的调用顺序。
//     NetSenderSubmission mixed;
//     SUBMIT_CHECK(mixed.start(1, 101) == NI_OK && mixed.ready(1, 128, 4096) == NI_OK);
//     for (uint32_t round = 0; round < 512; ++round)
//     {
//         uint64_t requests[8] = {};
//         uint8_t content[32] = {};
//         for (uint32_t index = 0; index < 8; ++index)
//         {
//             SUBMIT_CHECK(mixed.begin(101, round, 5000, requests[index]) == NI_OK);
//         }
//         for (uint32_t index = 0; index < 8; ++index)
//         {
//             std::memset(content, static_cast<int>(index + 1), sizeof(content));
//             SUBMIT_CHECK(mixed.send(requests[7 - index], content, sizeof(content)) == NI_OK);
//             frame.m_kind = index % 2 == 0 ? NI_DATA : NI_DATA_ACK;
//             frame.m_requestId = index + 1;
//             frame.m_payload = content;
//             frame.m_payloadSize = sizeof(content);
//             SUBMIT_CHECK(mixed.data(frame) == NI_OK);
//         }
//         std::memset(content, 0, sizeof(content));
//         SUBMIT_CHECK(mixed.beginPoll() == NI_OK);
//         for (uint32_t index = 0; index < 8; ++index)
//         {
//             SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN && command.m_id == requests[7 - index]);
//             mixed.finish(NI_OK);
//             SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
//             if ((round + index) % 3 == 0)
//             {
//                 mixed.finish(NI_LIMIT_EXCEEDED);
//                 SUBMIT_CHECK(mixed.next(command) == NI_NEED_MORE);
//                 SUBMIT_CHECK(mixed.beginPoll() == NI_OK && mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
//             }
//             std::memset(content, static_cast<int>(index + 1), sizeof(content));
//             SUBMIT_CHECK(command.m_id == requests[7 - index] && command.m_size == sizeof(content));
//             SUBMIT_CHECK(std::memcmp(command.m_bytes, content, sizeof(content)) == 0);
//             // 模拟协议调用尚未返回时先回报唯一终态。
//             SUBMIT_CHECK(mixed.complete(1, command.m_id, terminal));
//             SUBMIT_CHECK(std::memcmp(command.m_bytes, content, sizeof(content)) == 0);
//             mixed.finish(NI_OK);
//             SUBMIT_CHECK(mixed.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
//             SUBMIT_CHECK(command.m_frame.m_kind == (index % 2 == 0 ? NI_DATA : NI_DATA_ACK));
//             SUBMIT_CHECK(command.m_frame.m_requestId == index + 1 && command.m_frame.m_payloadSize == sizeof(content));
//             SUBMIT_CHECK(std::memcmp(command.m_frame.m_payload, content, sizeof(content)) == 0);
//             mixed.finish(NI_OK);
//         }
//         SUBMIT_CHECK(mixed.next(command) == NI_NEED_MORE && mixed.idle() && mixed.queuedBytes() == 0);
//     }
//
// // 在每个借用阶段退出，独立验证请求、流帧、未读回执均由所属模块回收。
// #if defined(_MSC_VER) && defined(_DEBUG)
//     _CrtMemState heapBefore = {}, heapAfter = {};
//     _CrtMemCheckpoint(&heapBefore);
// #endif
//     for (uint32_t round = 0; round < 1024; ++round)
//     {
//         NetSenderSubmission ending;
//         uint8_t owned[256] = {};
//         std::memset(owned, 0x3d, sizeof(owned));
//         SUBMIT_CHECK(ending.start(1, 101) == NI_OK && ending.ready(1, 128, 4096) == NI_OK);
//         SUBMIT_CHECK(ending.begin(101, 11, 5000, low) == NI_OK);
//         const uint32_t size = round % 4 == 0 ? 0 : sizeof(owned);
//         SUBMIT_CHECK(ending.sendSynchronous(low, size == 0 ? nullptr : owned, size, ticket) == NI_OK);
//         frame.m_kind = round % 2 == 0 ? NI_DATA : NI_DATA_ACK;
//         frame.m_payload = owned;
//         frame.m_payloadSize = sizeof(owned);
//         SUBMIT_CHECK(ending.data(frame) == NI_OK);
//         SUBMIT_CHECK(ending.beginPoll() == NI_OK && ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_BEGIN);
//         ending.finish(NI_OK);
//         SUBMIT_CHECK(ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_SEND);
//         SUBMIT_CHECK(command.m_size == size && (size == 0 ? command.m_bytes == nullptr : std::memcmp(command.m_bytes, owned, size) == 0));
//         if (round % 4 == 0)
//         {
//             ending.disconnected(1);
//             SUBMIT_CHECK(ending.complete(1, low, terminal));
//             // 模拟外层异常兜底释放，故意保留当前借用和未读失败回执。
//         }
//         else if (round % 4 == 1)
//         {
//             SUBMIT_CHECK(ending.complete(1, low, terminal));
//             ending.finish(NI_OK);
//             ending.advanceSynchronous(NI_OK);
//             SUBMIT_CHECK(ending.synchronousResult(ticket, false) == NI_OK);
//         }
//         else if (round % 4 == 2)
//         {
//             ending.close(NI_CANCELLED);
//             ending.finish(NI_LIMIT_EXCEEDED);
//         }
//         else
//         {
//             ending.finish(NI_OK);
//             ending.advanceSynchronous(NI_OK);
//             SUBMIT_CHECK(ending.synchronousResult(ticket, true) == NI_OK);
//             SUBMIT_CHECK(ending.next(command) == NI_READY && command.m_kind == NET_SUBMIT_DATA);
//             ending.finish(NI_LIMIT_EXCEEDED);
//         }
//     }
// #if defined(_MSC_VER) && defined(_DEBUG)
//     _CrtMemCheckpoint(&heapAfter);
//     SUBMIT_CHECK(heapAfter.lCounts[_NORMAL_BLOCK] == heapBefore.lCounts[_NORMAL_BLOCK]);
//     SUBMIT_CHECK(heapAfter.lSizes[_NORMAL_BLOCK] == heapBefore.lSizes[_NORMAL_BLOCK]);
//     SUBMIT_CHECK(heapAfter.lCounts[_CLIENT_BLOCK] == heapBefore.lCounts[_CLIENT_BLOCK]);
//     SUBMIT_CHECK(heapAfter.lSizes[_CLIENT_BLOCK] == heapBefore.lSizes[_CLIENT_BLOCK]);
//     std::printf("SUBMISSION_CLEANUP cycles=1024 live_blocks_delta=0 live_bytes_delta=0\n");
// #endif
//     std::printf("NetSenderSubmission FIFO and lifetime checks=%u PASS\n", checks);
//      return 0;
//  #undef SUBMIT_CHECK
//  }
// NET_SENDER_SUBMISSION_UNIT_END