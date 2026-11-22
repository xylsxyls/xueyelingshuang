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