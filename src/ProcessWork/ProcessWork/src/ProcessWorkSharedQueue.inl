#pragma once
#include "ProcessWorkAtomic.h"

inline bool ProcessWorkSharedQueue::enqueue(ProcessWorkProducerState& state, int32_t bufferIndex, bool inlined,
    uint32_t extent, uint32_t& hint, uint32_t granted, bool* contended)
{
    if (bufferIndex <= 0 || bufferIndex > 0xffffff || extent > 0xffffff || state.m_shared == nullptr)
    {
        return false;
    }
    ProcessWorkSharedStream& stream = *state.m_shared;
    const int64_t tail = ProcessWorkAtomic::load(&stream.m_writeNode);
    if (state.m_tailNode == nullptr || state.m_tail != tail)
    {
        state.m_tailNode = node(static_cast<uint32_t>(tail) - 1);
        state.m_tail = tail;
    }
    ProcessWorkSharedNode* last = state.m_tailNode;
    const int32_t offset = static_cast<int32_t>(ProcessWorkAtomic::load(&stream.m_writeOffset));
    if (last == nullptr || offset < 0 || offset > kProcessWorkSegmentCells)
    {
        return false;
    }
    const int64_t payload = bufferIndex | (inlined ? 0x1000000 : 0);
    if (offset < kProcessWorkSegmentCells)
    {
        ProcessWorkAtomic::store(&last->m_cells[offset], static_cast<int64_t>(static_cast<uint64_t>(tail) & 0xffffffff00000000ULL) | payload);
        // 本物理线程是唯一发布者；发布完整正文后才更新自己的提示，不额外竞争CAS。
        ProcessWorkAtomic::store(&stream.m_writeOffset, int64_t(offset + 1));
        return true;
    }
    return enqueueSegment(state, *last, tail, payload, extent, hint, granted, contended);
}

inline void ProcessWorkSharedQueue::releaseBody(uint32_t index)
{
    ProcessWorkSharedBlock* body = m_storage->block(index);
    if (body != nullptr)
    {
        releaseBody(index, *body);
    }
}

inline void ProcessWorkSharedQueue::releaseBody(uint32_t index, ProcessWorkSharedBlock& body)
{
    if (hasWaiters())
    {
        releaseBodyToWaiter(index, body);
    }
    else
    {
        // 请求可能在此检查后才发布，接收器下一轮service仍会扫描并授予这个FREE槽。
        ProcessWorkAtomic::store(&body.m_owner, int64_t(0));
    }
}

inline bool ProcessWorkSharedQueue::hasWaiters() const
{
    return ProcessWorkAtomic::load(&m_control->m_requestEpoch) != ProcessWorkAtomic::load(&m_control->m_servedEpoch);
}

inline uint32_t ProcessWorkSharedQueue::nodeCount(uint32_t extent) const
{
    const int64_t streams = ProcessWorkAtomic::load(&m_control->m_nextStream);
    return extent + 1 + static_cast<uint32_t>(streams < 0xffffff ? streams : 0xffffff);
}

inline uint32_t ProcessWorkSharedQueue::nodeAt(uint32_t ordinal, uint32_t extent) const
{
    return ordinal <= extent ? ordinal : 0x1000000U + ordinal - extent;
}

inline ProcessWorkSharedNode* ProcessWorkSharedQueue::node(uint32_t index)
{
    if (index <= 0xffffff)
    {
        return m_storage->node(index);
    }
    ProcessWorkSharedStream* stream = index <= 0x1ffffff ? m_storage->stream(index - 0x1000000) : nullptr;
    return stream == nullptr ? nullptr : &stream->m_first;
}

inline bool ProcessWorkSharedQueue::front(uint32_t& slot, int32_t& bufferIndex)
{
    bool inlined = false;
    return front(slot, bufferIndex, inlined);
}

inline bool ProcessWorkSharedQueue::readFront(ProcessWorkReadState& reader, uint32_t& slot, int32_t& bufferIndex, bool& inlined)
{
    ProcessWorkSharedStream& stream = *reader.m_shared;
    if (reader.m_node == nullptr)
    {
        if (ProcessWorkAtomic::load(&stream.m_owner) <= 0)
        {
            return false;
        }
        reader.m_head = ProcessWorkAtomic::load(&stream.m_head);
        const uint32_t encoded = static_cast<uint32_t>(reader.m_head);
        reader.m_node = encoded == 0 ? nullptr : node(encoded - 1);
    }
    ProcessWorkSharedNode* current = reader.m_node;
    if (current == nullptr)
    {
        return false;
    }
    if (stream.m_headSlot == kProcessWorkSegmentCells)
    {
        const uint32_t next = static_cast<uint32_t>(ProcessWorkAtomic::load(&current->m_link));
        if (next != 0)
        {
            ProcessWorkSharedNode* following = node(next - 1);
            if (following == nullptr || next == static_cast<uint32_t>(reader.m_head))
            {
                bufferIndex = -1;
                return true;
            }
            const int64_t id = static_cast<int64_t>((static_cast<uint64_t>(ProcessWorkAtomic::load(&following->m_link)) & 0xffffffff00000000ULL) | next);
            ProcessWorkAtomic::store(&following->m_owner, int64_t(-2));
            ProcessWorkAtomic::store(&stream.m_head, id);
            stream.m_headSlot = 0;
            const uint32_t retired = static_cast<uint32_t>(reader.m_head) - 1;
            reader.m_head = id;
            reader.m_node = following;
            releaseNode(retired);
            current = following;
        }
    }
    const int32_t position = static_cast<int32_t>(stream.m_headSlot);
    if (position < 0 || position > kProcessWorkSegmentCells)
    {
        bufferIndex = -1;
        return true;
    }
    const uint64_t cell = position == kProcessWorkSegmentCells ? 0 :
        static_cast<uint64_t>(ProcessWorkAtomic::load(&current->m_cells[position]));
    if (position < kProcessWorkSegmentCells && (cell >> 32) != (static_cast<uint64_t>(reader.m_head) >> 32))
    {
        bufferIndex = -1;
        return true;
    }
    const uint32_t payload = static_cast<uint32_t>(cell);
    if (payload == 0)
    {
        if (ProcessWorkAtomic::load(&stream.m_dead) != 0 && ProcessWorkAtomic::load(&stream.m_request) == 0)
        {
            const uint32_t retired = static_cast<uint32_t>(reader.m_head) - 1;
            ProcessWorkAtomic::store(&stream.m_owner, int64_t(0));
            ProcessWorkAtomic::store(&stream.m_head, int64_t(0));
            reader.m_node = nullptr;
            reader.m_head = 0;
            m_compactStreams = true;
            releaseNode(retired);
        }
        return false;
    }
    slot = (static_cast<uint32_t>(reader.m_head) - 1) * kProcessWorkSegmentCells + static_cast<uint32_t>(position) + 1;
    bufferIndex = (payload & 0xfe000000U) != 0 ? -1 : static_cast<int32_t>(payload & 0xffffffU);
    inlined = (payload & 0x1000000U) != 0;
    return true;
}

inline void ProcessWorkSharedQueue::advanceRead()
{
    ++m_readCursor;
    if (m_readCursor >= m_roundCount)
    {
        m_readCursor = 0;
        m_roundCount = m_active.size();
    }
    m_readQuota = 0;
    m_readBytes = 0;
}

inline bool ProcessWorkSharedQueue::front(uint32_t& slot, int32_t& bufferIndex, bool& inlined)
{
    if (m_frontOffset >= 0 && m_frontReader != nullptr)
    {
        return readFront(*m_frontReader, slot, bufferIndex, inlined);
    }
    if (ProcessWorkAtomic::load(&m_control->m_streamEpoch) != m_seenStreamEpoch)
    {
        refreshStreams();
    }
    const size_t count = m_active.size();
    if (m_roundCount == 0)
    {
        m_roundCount = count;
    }
    // 旧轮次结束后才纳入新流；本次可能先走旧轮余量，再走一次新轮。
    // 只扫描count次会重复旧轮前缀却漏掉新流，把已有完整正文误报为空。
    // count在本次调用固定，最多两轮，持续新增流不能无限延长扫描。
    const size_t scanLimit = count + m_roundCount;
    for (size_t scanned = 0; scanned < scanLimit; ++scanned)
    {
        if (m_readCursor >= count)
        {
            m_readCursor = 0;
        }
        ProcessWorkReadState* reader = m_active[m_readCursor];
        if (readFront(*reader, slot, bufferIndex, inlined))
        {
            m_frontReader = reader;
            m_frontSlot = slot;
            m_frontOffset = static_cast<int32_t>(reader->m_shared->m_headSlot);
            return true;
        }
        advanceRead();
    }
    return false;
}

inline bool ProcessWorkSharedQueue::consume(uint32_t slot, uint32_t bytes)
{
    if (m_frontOffset < 0 || m_frontReader == nullptr || m_frontSlot != slot ||
        m_frontReader->m_shared->m_headSlot != m_frontOffset)
    {
        return false;
    }
    m_frontReader->m_shared->m_headSlot = m_frontOffset + 1;
    m_frontOffset = -1;
    m_frontReader = nullptr;
    m_readBytes += bytes;
    if (++m_readQuota == 32 || m_readBytes >= 256 * 1024)
    {
        advanceRead();
    }
    return true;
}