#include "NetFileTransfer.h"
#include "NetPeerMacro.h"
#include "NetFileTransferState.h"
#include <algorithm>
#include <cstring>

NetFileTransfer::NetFileTransfer() :
m_state(nullptr)
{

}

NetFileTransfer::~NetFileTransfer()
{
    // 宿主先cancel并驱动done；共享队列的存活及最终停止由宿主负责。
    if (m_state != nullptr && m_state->m_phase != NET_FILE_TRANSFER_DONE)
    {
        cancel();
    }
    delete m_state;
}

int32_t NetFileTransfer::start(NetFileIo& io, uint64_t file, const NetInterfaceFileInfo& info, uint32_t product, bool sender,
    const char* temporary, const char* destination, uint64_t nowMs, bool replace)
{
    if (m_state != nullptr || file == 0 || io.done() || info.state != NI_FILE_TRANSFERRING || product == 0 ||
        (!sender && destination != nullptr && *destination != 0 && (temporary == nullptr || *temporary == 0)))
    {
        return NI_INVALID_ARGUMENT;
    }
    uint32_t lengths[2] = {};
    const char* paths[2] = {temporary, destination};
    for (uint32_t index = 0; index < 2; ++index)
    {
        if (paths[index] == nullptr)
        {
            continue;
        }
        while (lengths[index] <= 4096 && paths[index][lengths[index]] != 0)
        {
            ++lengths[index];
        }
        if (lengths[index] > 4096)
        {
            return NI_INVALID_ARGUMENT;
        }
    }
    uint8_t encoded[NET_INTERFACE_FILE_INFO_SIZE] = {};
    const int32_t valid = NetInterfaceFile::encodeInfo(info, encoded, sizeof(encoded));
    NetInterface::wipe(encoded, sizeof(encoded));
    if (valid != NI_OK)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::unique_ptr<NetFileTransferState> state(new NetFileTransferState());
        state->m_flow.reset(new NetInterfaceFileFlow(info, product, sender, nowMs));
        const int32_t progress = state->m_flow->progress(nowMs);
        if (progress < 0)
        {
            return progress;
        }
        if (lengths[0] != 0)
        {
            state->m_temporary.assign(temporary, lengths[0]);
        }
        if (lengths[1] != 0)
        {
            state->m_destination.assign(destination, lengths[1]);
        }
        state->m_io = &io;
        state->m_file = file;
        state->m_resume = info.offset;
        state->m_sender = sender;
        state->m_replace = replace;
        state->m_now = nowMs;
        m_state = state.release();
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetFileTransfer::prepare(uint32_t operation, uint64_t offset, const uint8_t* bytes, uint32_t size)
{
    NetFileIoRequest& request = m_state->m_waiting;
    request = NetFileIoRequest();
    request.file = m_state->m_file;
    request.operation = operation;
    request.offset = offset;
    request.bytes = bytes;
    request.size = size;
    request.flags = operation == NET_FILE_IO_WRITE || (operation == NET_FILE_IO_PUBLISH && m_state->m_replace) ? 1 : 0;
    request.path = m_state->m_temporary.c_str();
    request.destination = m_state->m_destination.c_str();
}

int32_t NetFileTransfer::submit()
{
    NetFileIoRequest& request = m_state->m_waiting;
    int32_t result = m_state->m_io->nextRequestId(request.id);
    if (result != NI_OK)
    {
        return result;
    }
    result = m_state->m_io->submit(request);
    if (result == NI_OK)
    {
        m_state->m_job = request;
        // submit接纳已复制全部输入；完成校验只使用身份，不再持有这些借用。
        m_state->m_job.bytes = nullptr;
        m_state->m_job.path = nullptr;
        m_state->m_job.destination = nullptr;
        request = NetFileIoRequest();
    }
    else
    {
        // 被拒绝也消耗编号；背压重试必须向共享队列申请新编号。
        request.id = 0;
    }
    return result;
}

int32_t NetFileTransfer::poll(uint64_t nowMs)
{
    if (m_state == nullptr)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_phase == NET_FILE_TRANSFER_DONE)
    {
        return m_state->m_failure < 0 ? m_state->m_failure : NI_READY;
    }
    m_state->m_now = nowMs;
    const int32_t progress = m_state->m_flow->progress(nowMs);
    if (progress < 0 && m_state->m_failure == NI_OK)
    {
        cancel(progress);
    }
    if (m_state->m_io->done() && (m_state->m_file != 0 || m_state->m_job.id != 0))
    {
        // 后端done保证所有句柄已经释放；不再给它投递虚假的CLOSE。
        m_state->m_file = 0;
        m_state->m_job = NetFileIoRequest();
        cancel(NI_CANCELLED);
        m_state->m_phase = NET_FILE_TRANSFER_DONE;
        return m_state->m_failure < 0 ? m_state->m_failure : NI_READY;
    }
    if (m_state->m_job.id != 0)
    {
        return NI_NEED_MORE;
    }
    if (m_state->m_sender && progress == NI_READY && m_state->m_phase != NET_FILE_TRANSFER_STOPPING)
    {
        // 对端最终ACK已成功，剩余职责只有释放本地只读句柄。
        cancel(NI_OK);
    }
    if (m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
    {
        if (m_state->m_file == 0)
        {
            m_state->m_phase = NET_FILE_TRANSFER_DONE;
            return m_state->m_failure < 0 ? m_state->m_failure : NI_READY;
        }
        if (m_state->m_waiting.operation == 0)
        {
            prepare(NET_FILE_IO_CLOSE);
        }
    }
    else if (m_state->m_phase == NET_FILE_TRANSFER_DRAIN)
    {
        NetInterfaceFrame output = {};
        if (m_state->m_flow->peekOutput(output) == NI_NEED_MORE)
        {
            m_state->m_phase = NET_FILE_TRANSFER_DONE;
            return NI_READY;
        }
        return NI_NEED_MORE;
    }
    else if (m_state->m_waiting.operation == 0)
    {
        if (m_state->m_phase == NET_FILE_TRANSFER_FLUSH)
        {
            prepare(NET_FILE_IO_FLUSH);
        }
        else if (m_state->m_phase == NET_FILE_TRANSFER_FINALIZE)
        {
            // 自清理临时文件必须持句柄发布，禁止先CLOSE再按路径重新找文件。
            prepare(m_state->m_destination.empty() ? NET_FILE_IO_CLOSE : NET_FILE_IO_PUBLISH);
        }
        else if (m_state->m_prime < m_state->m_resume)
        {
            prepare(NET_FILE_IO_READ, m_state->m_prime, nullptr,
                static_cast<uint32_t>((std::min<uint64_t>)(UINT64_C(65536), m_state->m_resume - m_state->m_prime)));
        }
        else if (m_state->m_sender)
        {
            uint64_t offset = 0;
            uint32_t size = 0;
            if (m_state->m_flow->readRequest(offset, size) == NI_READY)
            {
                prepare(NET_FILE_IO_READ, offset, nullptr, size);
            }
        }
        else
        {
            uint64_t offset = 0;
            uint32_t size = 0;
            const uint8_t* bytes = nullptr;
            if (m_state->m_flow->writeRequest(offset, bytes, size) == NI_READY)
            {
                prepare(NET_FILE_IO_WRITE, offset, bytes, size);
            }
            else if (m_state->m_flow->takeCommit() == NI_READY)
            {
                m_state->m_phase = NET_FILE_TRANSFER_FLUSH;
                prepare(NET_FILE_IO_FLUSH);
            }
        }
    }
    if (m_state->m_waiting.operation == 0)
    {
        return NI_OK;
    }
    const int32_t submitted = submit();
    if (submitted == NI_LIMIT_EXCEEDED)
    {
        return NI_NEED_MORE;
    }
    if (submitted < 0)
    {
        cancel(submitted);
    }
    return m_state->m_failure < 0 ? m_state->m_failure : NI_OK;
}

bool NetFileTransfer::onIoResult(const NetFileIoResult& result)
{
    if (m_state == nullptr || m_state->m_job.id == 0 || result.id != m_state->m_job.id)
    {
        return false;
    }
    const NetFileIoRequest expected = m_state->m_job;
    m_state->m_job = NetFileIoRequest();
    if (expected.operation == NET_FILE_IO_CLOSE || expected.operation == NET_FILE_IO_PUBLISH)
    {
        // 已接纳的释放类工作即使失败或取消也消费文件号；不可再次关闭可能复用的号。
        m_state->m_file = 0;
    }
    if (result.file != expected.file || result.operation != expected.operation || result.offset != expected.offset ||
        result.result > NI_OK || (result.result == NI_OK && expected.operation == NET_FILE_IO_READ &&
        (result.size != expected.size || result.bytes == nullptr)))
    {
        cancel(NI_INVALID_STATE);
        return true;
    }
    if (result.result != NI_OK)
    {
        cancel(result.result);
        return true;
    }
    if (m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
    {
        return true;
    }
    int32_t accepted = NI_OK;
    if (expected.operation == NET_FILE_IO_READ)
    {
        if (m_state->m_prime < m_state->m_resume)
        {
            accepted = m_state->m_flow->prime(expected.offset, result.bytes, result.size);
            if (accepted == NI_OK)
            {
                m_state->m_prime += result.size;
            }
        }
        else
        {
            accepted = m_state->m_flow->produced(expected.offset, result.bytes, result.size);
        }
    }
    else if (expected.operation == NET_FILE_IO_WRITE)
    {
        // WRITE的result.size是输出正文长度，通常为0，不是写入长度。
        accepted = m_state->m_flow->written(expected.offset, expected.size, NI_OK);
    }
    else if (expected.operation == NET_FILE_IO_FLUSH)
    {
        m_state->m_phase = NET_FILE_TRANSFER_FINALIZE;
    }
    else
    {
        accepted = m_state->m_flow->committed(NI_OK);
        if (accepted >= 0)
        {
            m_state->m_phase = NET_FILE_TRANSFER_DRAIN;
        }
    }
    if (accepted < 0)
    {
        cancel(accepted);
    }
    return true;
}

int32_t NetFileTransfer::accept(const NetInterfaceFrame& frame)
{
    if (m_state == nullptr)
    {
        return NI_INVALID_STATE;
    }
    const int32_t result = m_state->m_flow->accept(frame);
    if (result < 0 && result != NI_LIMIT_EXCEEDED && result != NI_NOT_FOUND)
    {
        cancel(result);
    }
    return result;
}

int32_t NetFileTransfer::peekOutput(NetInterfaceFrame& frame)
{
    if (m_state == nullptr)
    {
        std::memset(&frame, 0, sizeof(frame));
        return NI_INVALID_STATE;
    }
    return m_state->m_flow->peekOutput(frame);
}

int32_t NetFileTransfer::consumeOutput()
{
    return m_state != nullptr ? m_state->m_flow->consumeOutput() : NI_INVALID_STATE;
}

void NetFileTransfer::cancel(int32_t reason)
{
    if (m_state == nullptr || m_state->m_phase == NET_FILE_TRANSFER_DONE || m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
    {
        return;
    }
    // 协议已经成功后只收敛资源；晚到的取消不把已提交文件或已确认发送改成失败。
    const int32_t status = m_state->m_flow->progress(m_state->m_now);
    if (status != NI_READY)
    {
        m_state->m_failure = status < 0 ? status : reason < 0 ? reason : NI_CANCELLED;
        m_state->m_flow->cancel(m_state->m_failure);
    }
    m_state->m_phase = NET_FILE_TRANSFER_STOPPING;
    m_state->m_waiting = NetFileIoRequest();
    if (m_state->m_job.id != 0)
    {
        m_state->m_io->cancel(m_state->m_job.id);
    }
}

bool NetFileTransfer::done() const
{
    return m_state == nullptr || m_state->m_phase == NET_FILE_TRANSFER_DONE;
}

void NetFileTransfer::snapshot(NetInterfaceFileInfo& info, int32_t& result) const
{
    std::memset(&info, 0, sizeof(info));
    result = NI_INVALID_STATE;
    if (m_state != nullptr)
    {
        m_state->m_flow->snapshot(info, result);
    }
}

// NET_FILE_TRANSFER_UNIT_BEGIN
// #include "NetPeer.h"
// #include "NetPeer/NetFileTransferTest.h"
// #include <algorithm>
// #include <cstdio>
// #include <cstring>
// #if defined(_WIN32) && defined(_DEBUG)
// #include <crtdbg.h>
// #endif
//
// #define TRANSFER_VERIFY(condition) { ++m_checks; if (!(condition)) { std::printf("FAIL %u %s\n", static_cast<uint32_t>(__LINE__), #condition); return false; } }
// uint32_t NetFileTransferTest::m_checks = 0;
// NetFileTransferTestBackend* NetFileTransferTestBackend::m_current = nullptr;
//
// NetFileTransferTestBackend::NetFileTransferTestBackend() :
// m_request(), m_result(), m_next(0), m_attempted(0), m_busy(2), m_rejected(0), m_released(0),
// m_flushed(0), m_published(0), m_failOperation(0), m_durable(0), m_ready(false), m_cancelled(false),
// m_stopped(false), m_file(true), m_bad(false)
// {
//     m_current = this;
// }
// NetFileTransferTestBackend::~NetFileTransferTestBackend()
// {
//     if (m_current == this) { m_current = nullptr; }
// }
// int32_t NetFileTransferTestBackend::init()
//{
//    return NI_OK;
//}

// int32_t NetFileTransferTestBackend::nextRequestId(uint64_t& id)
//{
//    id = ++m_next; return NI_OK;
//}

// int32_t NetFileTransferTestBackend::submit(const NetFileIoRequest& request)
// {
//     if (request.id <= m_attempted || request.file != 7 || !m_file || m_stopped) { m_bad = true; return NI_INVALID_STATE; }
//     m_attempted = request.id;
//     if (m_request.id != 0 || m_busy != 0)
//     {
//         if (m_busy != 0) { --m_busy; }
//         ++m_rejected;
//         return NI_LIMIT_EXCEEDED;
//     }
//     m_busy = 2;
//     m_request = request;
//     if (request.size != 0 && request.bytes != nullptr) { m_input.assign(request.bytes, request.bytes + request.size); }
//     m_request.bytes = nullptr;
//     m_cancelled = false;
//     m_ready = false;
//     return NI_OK;
// }
// int32_t NetFileTransferTestBackend::cancel(uint64_t id)
// {
//     if (m_request.id != id) { return NI_NOT_FOUND; }
//     m_cancelled = true;
//     return NI_OK;
// }
// int32_t NetFileTransferTestBackend::peek(NetFileIoResult& result)
// {
//     result = NetFileIoResult();
//     if (m_request.id == 0) { return NI_NEED_MORE; }
//     if (!m_ready)
//     {
//         m_result = NetFileIoResult();
//         m_result.id = m_request.id;
//         m_result.file = m_request.file;
//         m_result.operation = m_request.operation;
//         m_result.offset = m_request.offset;
//         m_result.result = m_cancelled ? NI_CANCELLED : m_failOperation == m_request.operation ? NI_INVALID_STATE : NI_OK;
//         if (m_request.operation == NET_FILE_IO_CLOSE || m_request.operation == NET_FILE_IO_PUBLISH)
//         {
//             m_bad = m_bad || !m_file;
//             m_file = false;
//             ++m_released;
//         }
//         if (m_result.result == NI_OK)
//         {
//             switch (m_request.operation)
//             {
//             case NET_FILE_IO_READ:
//                 if (m_request.offset > m_content.size() || m_request.size > m_content.size() - m_request.offset)
//                 {
//                     m_bad = true;
//                     m_result.result = NI_INVALID_STATE;
//                     break;
//                 }
//                 m_output.assign(m_content.begin() + static_cast<size_t>(m_request.offset),
//                     m_content.begin() + static_cast<size_t>(m_request.offset) + m_request.size);
//                 m_result.bytes = m_output.data();
//                 m_result.size = static_cast<uint32_t>(m_output.size());
//                 break;
//             case NET_FILE_IO_WRITE:
//                 m_bad = m_bad || m_request.flags != 1 || m_input.size() != m_request.size || m_request.offset != m_content.size();
//                 m_content.insert(m_content.end(), m_input.begin(), m_input.end());
//                 m_durable = m_content.size();
//                 break;
//             case NET_FILE_IO_FLUSH:
//                 ++m_flushed;
//                 break;
//             case NET_FILE_IO_PUBLISH:
//                 m_bad = m_bad || m_flushed != 1 || m_request.path == nullptr || m_request.destination == nullptr;
//                 ++m_published;
//                 break;
//             case NET_FILE_IO_CLOSE:
//                 break;
//             default:
//                 m_bad = true;
//                 m_result.result = NI_INVALID_STATE;
//                 break;
//             }
//         }
//         m_ready = true;
//     }
//     result = m_result;
//     return NI_READY;
// }
// int32_t NetFileTransferTestBackend::consume(uint64_t id)
// {
//     if (!m_ready || id != m_request.id) { return NI_INVALID_STATE; }
//     NetInterface::wipe(m_input.empty() ? nullptr : m_input.data(), static_cast<uint32_t>(m_input.size()));
//     NetInterface::wipe(m_output.empty() ? nullptr : m_output.data(), static_cast<uint32_t>(m_output.size()));
//     m_input.clear(); m_output.clear(); m_request = NetFileIoRequest(); m_result = NetFileIoResult(); m_ready = false;
//     return NI_OK;
// }
// void NetFileTransferTestBackend::stop()
//{
//    m_stopped = true; m_file = false;
//}

// bool NetFileTransferTestBackend::done() const
// {
//     return m_stopped;
// }

// void NetFileTransferTestBackend::uninit()
//{
//    m_stopped = true;
//}

// NetFileIoBackend* NetFileTransferTestBackend::create()
//{
//    return new NetFileTransferTestBackend();
//}

//
// NetInterfaceFileInfo NetFileTransferTest::info(const std::vector<uint8_t>& bytes, uint32_t chunk, uint32_t resume)
// {
//     NetInterfaceFileInfo metadata = {};
//     metadata.direction = 1; metadata.state = NI_FILE_TRANSFERRING; metadata.chunk = chunk; metadata.window = 2;
//     metadata.total = bytes.size(); metadata.offset = resume; metadata.stream = 7; metadata.expiresUtc = 4000000000;
//     metadata.transfer[0] = 1; metadata.token[0] = 2;
//     NetInterface::sha256(bytes.empty() ? nullptr : bytes.data(), static_cast<uint32_t>(bytes.size()), metadata.hash, 32);
//     return metadata;
// }
//
// bool NetFileTransferTest::drain(NetFileIo& io, NetFileTransfer& flow)
// {
//     NetFileIoResult result = {};
//     const int32_t status = io.peek(result);
//     if (status == NI_NEED_MORE) { return true; }
//     TRANSFER_VERIFY(status == NI_READY);
//     NetFileIoResult foreign = result;
//     foreign.id = UINT64_MAX;
//     TRANSFER_VERIFY(!flow.onIoResult(foreign));
//     TRANSFER_VERIFY(flow.onIoResult(result));
//     TRANSFER_VERIFY(!flow.onIoResult(result));
//     TRANSFER_VERIFY(io.consume(result.id) == NI_OK);
//     return true;
// }
//
// bool NetFileTransferTest::transfer(uint32_t total, uint32_t chunk, uint32_t resume, bool publish,
//     uint32_t cancelOperation, bool after, uint32_t failOperation)
// {
//     std::vector<uint8_t> content(total);
//     for (uint32_t i = 0; i < total; ++i) { content[i] = static_cast<uint8_t>(i * 17 + i / 19); }
//     NetFileIo sending, receiving;
//     TRANSFER_VERIFY(sending.init() == NI_OK);
//     NetFileTransferTestBackend* source = NetFileTransferTestBackend::m_current;
//     source->m_content = content;
//     TRANSFER_VERIFY(receiving.init() == NI_OK);
//     NetFileTransferTestBackend* target = NetFileTransferTestBackend::m_current;
//     target->m_content.assign(content.begin(), content.begin() + resume);
//     target->m_durable = resume;
//     target->m_failOperation = failOperation;
//     NetInterfaceFileInfo metadata = info(content, chunk, resume);
//     NetFileTransfer sender, receiver;
//     TRANSFER_VERIFY(sender.start(sending, 7, metadata, 9, true, nullptr, nullptr, 1000) == NI_OK);
//     TRANSFER_VERIFY(receiver.start(receiving, 7, metadata, 9, false, "/unit/part", publish ? "/unit/final" : nullptr, 1000) == NI_OK);
//     bool cancelled = false;
//     bool receiverWasCommitted = false;
//     bool failed = false;
//     uint32_t step = 0;
//     for (; step < 10000 && (!sender.done() || !receiver.done()); ++step)
//     {
//         const int32_t sent = sender.poll(1000 + step);
//         const int32_t received = receiver.poll(1000 + step);
//         TRANSFER_VERIFY((sent >= 0 && received >= 0) || cancelled || failOperation != 0);
//         const bool selected = !cancelled && cancelOperation != 0 &&
//             ((cancelOperation == NET_FILE_IO_READ ? source : target)->m_request.operation == cancelOperation);
//         if (selected && !after)
//         {
//             sender.cancel(); receiver.cancel(); cancelled = true;
//         }
//         TRANSFER_VERIFY(drain(sending, sender) && drain(receiving, receiver));
//         if (selected && after)
//         {
//             NetInterfaceFileInfo snapshot = {}; int32_t result = 0;
//             receiver.snapshot(snapshot, result);
//             receiverWasCommitted = result == NI_READY;
//             sender.cancel(); receiver.cancel(); cancelled = true;
//         }
//         if (failOperation != 0)
//         {
//             NetInterfaceFileInfo snapshot = {}; int32_t result = 0;
//             receiver.snapshot(snapshot, result);
//             if (result < 0) { failed = true; sender.cancel(); }
//         }
//         if (!cancelled && !failed)
//         {
//             NetInterfaceFrame frame = {};
//             if (sender.peekOutput(frame) == NI_READY)
//             {
//                 const int32_t result = receiver.accept(frame);
//                 TRANSFER_VERIFY(result == NI_OK || result == NI_LIMIT_EXCEEDED);
//                 if (result == NI_OK) { TRANSFER_VERIFY(sender.consumeOutput() == NI_OK); }
//             }
//             if (receiver.peekOutput(frame) == NI_READY)
//             {
//                 NetInterfaceFileAck ack = {};
//                 TRANSFER_VERIFY(NetInterfaceFile::parseAck(frame, ack) == NI_OK && ack.offset <= target->m_durable);
//                 TRANSFER_VERIFY(ack.flags != 1 || (!target->m_file && target->m_flushed == 1 && target->m_published == (publish ? 1U : 0U)));
//                 TRANSFER_VERIFY(sender.accept(frame) == NI_OK && receiver.consumeOutput() == NI_OK);
//             }
//         }
//     }
//     TRANSFER_VERIFY(step < 10000 && sender.done() && receiver.done());
//     TRANSFER_VERIFY(!source->m_bad && !target->m_bad && source->m_released == 1 && target->m_released == 1);
//     TRANSFER_VERIFY(source->m_rejected != 0 && target->m_rejected != 0);
//     NetInterfaceFileInfo snapshot = {}; int32_t result = 0;
//     receiver.snapshot(snapshot, result);
//     if (cancelOperation != 0)
//     {
//         TRANSFER_VERIFY(cancelled && result == (receiverWasCommitted ? NI_READY : NI_CANCELLED));
//     }
//     else if (failOperation != 0)
//     {
//         TRANSFER_VERIFY(failed && result == NI_INVALID_STATE);
//     }
//     else
//     {
//         TRANSFER_VERIFY(target->m_content == content && result == NI_READY && snapshot.offset == total && snapshot.state == NI_FILE_COMPLETED);
//         sender.cancel(); receiver.cancel();
//         TRANSFER_VERIFY(sender.poll(0) == NI_READY && receiver.poll(0) == NI_READY);
//     }
//     sending.stop(); receiving.stop(); sending.uninit(); receiving.uninit();
//     return true;
// }
//
// bool NetFileTransferTest::identity()
// {
//     for (uint32_t mode = 0; mode < 6; ++mode)
//     {
//         NetFileIo io;
//         TRANSFER_VERIFY(io.init() == NI_OK);
//         NetFileTransferTestBackend* backend = NetFileTransferTestBackend::m_current;
//         backend->m_content.resize(1024, 0x5a);
//         NetInterfaceFileInfo metadata = info(backend->m_content, 1024, 0);
//         NetFileTransfer flow;
//         TRANSFER_VERIFY(flow.start(io, 7, metadata, 9, true, nullptr, nullptr, 1000) == NI_OK);
//         for (uint32_t i = 0; i < 3; ++i) { TRANSFER_VERIFY(flow.poll(1000 + i) >= 0); }
//         NetFileIoResult result = {};
//         TRANSFER_VERIFY(io.peek(result) == NI_READY);
//         if (mode == 0) { ++result.file; }
//         if (mode == 1) { result.operation = NET_FILE_IO_WRITE; }
//         if (mode == 2) { ++result.offset; }
//         if (mode == 3) { --result.size; }
//         if (mode == 4) { result.bytes = nullptr; }
//         if (mode == 5) { result.result = NI_READY; }
//         TRANSFER_VERIFY(flow.onIoResult(result) && io.consume(result.id) == NI_OK);
//         NetInterfaceFrame output = {};
//         TRANSFER_VERIFY(flow.peekOutput(output) == NI_INVALID_STATE);
//         for (uint32_t i = 0; i < 10 && !flow.done(); ++i)
//         {
//             flow.poll(1003 + i);
//             TRANSFER_VERIFY(drain(io, flow));
//         }
//         TRANSFER_VERIFY(flow.done() && !backend->m_bad && backend->m_released == 1 && flow.poll(0) == NI_INVALID_STATE);
//         io.stop(); io.uninit();
//     }
//     return true;
// }
//
// bool NetFileTransferTest::run()
// {
//     m_checks = 0;
//     TRANSFER_VERIFY(NetFileIo::configureBackend(&NetFileTransferTestBackend::create) == NI_OK);
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState before; _CrtMemCheckpoint(&before);
// #endif
//     TRANSFER_VERIFY(identity());
//     const uint32_t lengths[] = {0, 1, 1024, 2357, 65537, 262145};
//     const uint32_t chunks[] = {1024, 65536};
//     for (uint32_t a = 0; a < 6; ++a)
//     {
//         for (uint32_t b = 0; b < 2; ++b)
//         {
//             for (uint32_t publish = 0; publish < 2; ++publish)
//             {
//                 TRANSFER_VERIFY(transfer(lengths[a], chunks[b], 0, publish != 0, 0, false, 0));
//                 TRANSFER_VERIFY(transfer(lengths[a], chunks[b], lengths[a], publish != 0, 0, false, 0));
//                 if (lengths[a] > chunks[b]) { TRANSFER_VERIFY(transfer(lengths[a], chunks[b], chunks[b], publish != 0, 0, false, 0)); }
//             }
//         }
//     }
//     const uint32_t operations[] = {NET_FILE_IO_READ, NET_FILE_IO_WRITE, NET_FILE_IO_FLUSH, NET_FILE_IO_PUBLISH, NET_FILE_IO_CLOSE};
//     for (uint32_t a = 0; a < 5; ++a)
//     {
//         for (uint32_t after = 0; after < 2; ++after)
//         {
//             TRANSFER_VERIFY(transfer(2357, 1024, 0, operations[a] != NET_FILE_IO_CLOSE, operations[a], after != 0, 0));
//         }
//         if (a != 0) { TRANSFER_VERIFY(transfer(2357, 1024, 0, operations[a] != NET_FILE_IO_CLOSE, 0, false, operations[a])); }
//     }
// #if defined(_WIN32) && defined(_DEBUG)
//     _CrtMemState after; _CrtMemCheckpoint(&after);
//     TRANSFER_VERIFY(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
// #endif
//     return true;
// }
//
// int main()
// {
//     if (!NetFileTransferTest::run())
//{
//    return 1;
//}

//     std::printf("File transfer checks=%u PASS\n", NetFileTransferTest::m_checks);
//     return 0;
// }
// #undef TRANSFER_VERIFY
// NET_FILE_TRANSFER_UNIT_END