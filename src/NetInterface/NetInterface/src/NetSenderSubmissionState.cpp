#include "NetSenderSubmissionState.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetSenderSubmission.h"
#include <chrono>
#include <cstring>
#include <new>

NetSenderSubmissionReceipt::NetSenderSubmissionReceipt() :
m_next(nullptr),
m_entry(nullptr),
m_ticket(0),
m_generation(0),
m_deadline(0),
m_result(NI_READY),
m_staged(false)
{

}

NetSenderSubmissionEntry::NetSenderSubmissionEntry() :
m_command(),
m_input(nullptr),
m_size(0),
m_phase(NET_SUBMISSION_RESERVED),
m_previousRequest(nullptr),
m_nextRequest(nullptr),
m_previousPending(nullptr),
m_nextPending(nullptr),
m_receipt(nullptr),
m_pending(false),
m_cancelled(false),
m_cancelTaken(false),
m_attempt(0)
{

}

NetSenderSubmissionEntry::~NetSenderSubmissionEntry()
{
    clear();
}

bool NetSenderSubmissionEntry::copy(const uint8_t* bytes, uint32_t size)
{
    uint8_t* input = size == 0 ? nullptr : new (std::nothrow) uint8_t[size];
    if (size != 0 && input == nullptr)
    {
        return false;
    }
    if (size != 0)
    {
        std::memcpy(input, bytes, size);
    }
    clear();
    m_input = input;
    m_size = size;
    return true;
}

void NetSenderSubmissionEntry::clear()
{
    NetInterface::wipe(m_input, m_size);
    delete[] m_input;
    m_input = nullptr;
    m_size = 0;
}

NetSenderSubmissionGuard::NetSenderSubmissionGuard(const NetSenderSubmissionGate& gate) :
m_gate(gate)
{
    if (m_gate.m_enter != nullptr)
    {
        m_gate.m_enter(m_gate.m_context);
    }
}

NetSenderSubmissionGuard::~NetSenderSubmissionGuard()
{
    if (m_gate.m_leave != nullptr)
    {
        m_gate.m_leave(m_gate.m_context);
    }
}

NetSenderSubmissionState::NetSenderSubmissionState() :
m_gate(),
m_gateConfigured(false),
m_generation(0),
m_product(0),
m_nextId(0),
m_nextFrame(0),
m_nextSync(0),
m_round(0),
m_limit(kRequests),
m_chunk(0),
m_bytes(0),
m_controlBytes(0),
m_requestCount(0),
m_frameCount(0),
m_controlCount(0),
m_receiptCount(0),
m_ready(false),
m_closing(false),
m_closeTaken(false),
m_reason(0),
m_requests(nullptr),
m_requestsTail(nullptr),
m_pending(nullptr),
m_pendingTail(nullptr),
m_taken(nullptr),
m_takenKind(NET_SUBMIT_NONE),
m_receipts(nullptr),
m_draining(nullptr)
{

}

NetSenderSubmissionState::~NetSenderSubmissionState()
{
    dropFrames();
    NetSenderSubmissionCommand unused = {};
    while (m_requests != nullptr)
    {
        take(m_requests, unused);
    }
    NetSenderSubmissionEntry* borrowed = m_taken;
    m_taken = nullptr;
    if (borrowed != nullptr)
    {
        retire(borrowed);
    }
    while (m_receipts != nullptr)
    {
        NetSenderSubmissionReceipt* receipt = m_receipts;
        m_receipts = receipt->m_next;
        delete receipt;
    }
}

NetSenderSubmissionEntry* NetSenderSubmissionState::find(uint64_t id) const
{
    for (NetSenderSubmissionEntry* entry = m_requests; entry != nullptr; entry = entry->m_nextRequest)
    {
        if (entry->m_command.m_id == id)
        {
            return entry;
        }
    }
    return nullptr;
}

void NetSenderSubmissionState::append(NetSenderSubmissionEntry* entry)
{
    entry->m_previousPending = m_pendingTail;
    entry->m_nextPending = nullptr;
    entry->m_pending = true;
    if (m_pendingTail != nullptr)
    {
        m_pendingTail->m_nextPending = entry;
    }
    else
    {
        m_pending = entry;
    }
    m_pendingTail = entry;
}

void NetSenderSubmissionState::remove(NetSenderSubmissionEntry* entry)
{
    if (!entry->m_pending)
    {
        return;
    }
    if (entry->m_previousPending != nullptr)
    {
        entry->m_previousPending->m_nextPending = entry->m_nextPending;
    }
    else
    {
        m_pending = entry->m_nextPending;
    }
    if (entry->m_nextPending != nullptr)
    {
        entry->m_nextPending->m_previousPending = entry->m_previousPending;
    }
    else
    {
        m_pendingTail = entry->m_previousPending;
    }
    entry->m_previousPending = nullptr;
    entry->m_nextPending = nullptr;
    entry->m_pending = false;
}

void NetSenderSubmissionState::clear(NetSenderSubmissionEntry* entry)
{
    const bool control = entry->m_command.m_kind == NET_SUBMIT_DATA && entry->m_command.m_frame.m_kind != NI_DATA;
    (control ? m_controlBytes : m_bytes) -= entry->m_size;
    entry->clear();
}

void NetSenderSubmissionState::retire(NetSenderSubmissionEntry* entry)
{
    remove(entry);
    if (entry->m_phase != NET_SUBMISSION_RETIRED)
    {
        if (entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            (entry->m_command.m_frame.m_kind == NI_DATA ? m_frameCount : m_controlCount)--;
        }
        entry->m_phase = NET_SUBMISSION_RETIRED;
    }
    // 回调可能先完成业务或断线，正文仍须覆盖当前协议调用的整个借用期。
    if (entry != m_taken)
    {
        clear(entry);
        if (entry->m_receipt != nullptr)
        {
            entry->m_receipt->m_entry = nullptr;
        }
        delete entry;
    }
}

void NetSenderSubmissionState::take(NetSenderSubmissionEntry* entry, NetSenderSubmissionCommand& command)
{
    command = entry->m_command;
    if (entry->m_previousRequest != nullptr)
    {
        entry->m_previousRequest->m_nextRequest = entry->m_nextRequest;
    }
    else
    {
        m_requests = entry->m_nextRequest;
    }
    if (entry->m_nextRequest != nullptr)
    {
        entry->m_nextRequest->m_previousRequest = entry->m_previousRequest;
    }
    else
    {
        m_requestsTail = entry->m_previousRequest;
    }
    entry->m_previousRequest = nullptr;
    entry->m_nextRequest = nullptr;
    --m_requestCount;
    retire(entry);
}

NetSenderSubmissionReceipt* NetSenderSubmissionState::createReceipt(uint64_t generation, uint64_t deadline)
{
    NetSenderSubmissionReceipt* receipt = new (std::nothrow) NetSenderSubmissionReceipt();
    if (receipt != nullptr)
    {
        receipt->m_ticket = m_nextSync + 1;
        receipt->m_generation = generation;
        receipt->m_deadline = deadline;
    }
    return receipt;
}

void NetSenderSubmissionState::addReceipt(NetSenderSubmissionReceipt* receipt, NetSenderSubmissionEntry* entry)
{
    receipt->m_entry = entry;
    receipt->m_next = m_receipts;
    m_receipts = receipt;
    entry->m_receipt = receipt;
    m_nextSync = receipt->m_ticket;
    ++m_receiptCount;
}

void NetSenderSubmissionState::settle(NetSenderSubmissionReceipt* receipt, int32_t result)
{
    if (receipt->m_result == NI_READY)
    {
        receipt->m_result = result;
    }
    if (m_draining == receipt)
    {
        m_draining = nullptr;
    }
}

void NetSenderSubmissionState::expireFrames(uint64_t nowMs)
{
    for (NetSenderSubmissionReceipt* receipt = m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        NetSenderSubmissionEntry* entry = receipt->m_entry;
        if (receipt->m_result == NI_READY && !receipt->m_staged && nowMs >= receipt->m_deadline &&
            entry != nullptr && entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            settle(receipt, NI_TIMEOUT);
            retire(entry);
        }
    }
}

void NetSenderSubmissionState::dropFrames()
{
    NetSenderSubmissionEntry* entry = m_pending;
    while (entry != nullptr)
    {
        NetSenderSubmissionEntry* next = entry->m_nextPending;
        if (entry->m_command.m_kind == NET_SUBMIT_DATA)
        {
            retire(entry);
        }
        entry = next;
    }
}

bool NetSenderSubmissionState::pendingReceipts() const
{
    for (NetSenderSubmissionReceipt* receipt = m_receipts; receipt != nullptr; receipt = receipt->m_next)
    {
        if (receipt->m_result == NI_READY)
        {
            return true;
        }
    }
    return false;
}