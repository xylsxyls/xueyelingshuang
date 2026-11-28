#include "NetFileTransfer.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetPeerMacro.h"
#include "NetFileTransferState.h"
#include <algorithm>
#include <cstring>

// 共享文件后端不向网络执行者通知完成或容量恢复。
static const uint32_t kFileIoPollMs = 100;

NetFileTransfer::NetFileTransfer() :
m_state(nullptr), m_writeBatchChunks(1), m_progressTimeout(15000)
{

}

NetFileTransfer::~NetFileTransfer()
{
    // 宿主先cancel并驱动done；共享队列的存活及最终停止由宿主负责。
    if (!done())
    {
        cancel();
    }
    delete m_state;
}

int32_t NetFileTransfer::setWriteBatchChunks(uint32_t chunks)
{
    if (m_state != nullptr) { return NI_INVALID_STATE; }
    if (chunks == 0 || chunks > 8) { return NI_INVALID_ARGUMENT; }
    m_writeBatchChunks = chunks;
    return NI_OK;
}

int32_t NetFileTransfer::setProgressTimeout(uint32_t timeoutMs)
{
    if (m_state != nullptr) { return NI_INVALID_STATE; }
    if (timeoutMs < 1000 || timeoutMs > 60000) { return NI_INVALID_ARGUMENT; }
    m_progressTimeout = timeoutMs;
    return NI_OK;
}

int32_t NetFileTransfer::start(NetFileIo& io, uint64_t file, const NetInterfaceFileInfo& info, uint32_t product, bool sender,
    const char* temporary, const char* destination, uint64_t nowMs, bool replace)
{
    if (m_state != nullptr || file == 0 || !io.accepting() || info.state != NI_FILE_TRANSFERRING || product == 0 ||
        m_writeBatchChunks > info.window || (sender && m_writeBatchChunks != 1) ||
        (!sender && destination != nullptr && *destination != 0 && (temporary == nullptr || *temporary == 0)))
    {
        return NI_INVALID_ARGUMENT;
    }
    const size_t lengths[] = {CStringManager::boundedLength(temporary, 4097),
        CStringManager::boundedLength(destination, 4097)};
    if (lengths[0] > 4096 || lengths[1] > 4096) { return NI_INVALID_ARGUMENT; }
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
        state->m_flow.reset(new NetInterfaceFileFlow(info, product, sender, nowMs, m_progressTimeout));
        const int32_t progress = state->m_flow->progress(nowMs);
        if (progress < 0)
        {
            return progress;
        }
        if (!sender && m_writeBatchChunks > 1)
        {
            const int32_t configured = state->m_flow->setWriteBatchChunks(m_writeBatchChunks);
            if (configured != NI_OK) { return configured; }
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
        NetExecution::progress();
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
    if (done())
    {
        return m_state->m_failure < 0 ? m_state->m_failure : NI_READY;
    }
    m_state->m_now = nowMs;
    const int32_t progress = m_state->m_flow->progress(nowMs);
    if (progress < 0 && m_state->m_failure == NI_OK)
    {
        cancel(progress);
    }
    // 后端工作线程已停止时，其完成队列仍可读取。等待宿主路由已接纳任务的真实结果。
    if (m_state->m_job.id != 0)
    {
        NetExecution::deadline(kFileIoPollMs, 0);
        return NI_NEED_MORE;
    }
    if (m_state->m_file != 0 && m_state->m_io->done())
    {
        // 后端done保证所有句柄已经释放；不再给它投递虚假的CLOSE。
        m_state->m_file = 0;
        cancel(NI_CANCELLED);
        // 最终 ACK 只确认远端提交。后端回收了尚未交给 CLOSE 的句柄，保留协议完成和本地首错。
        if (m_state->m_failure == NI_OK)
        {
            m_state->m_failure = NI_CANCELLED;
        }
        m_state->m_phase = NET_FILE_TRANSFER_DONE;
        return m_state->m_failure;
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
        const int32_t pending = m_state->m_flow->peekOutput(output);
        if (pending == NI_NEED_MORE)
        {
            m_state->m_phase = NET_FILE_TRANSFER_DONE;
            return NI_READY;
        }
        if (m_state->m_finalOutputSince == UINT64_MAX) { m_state->m_finalOutputSince = nowMs; }
        const uint64_t elapsed = nowMs - m_state->m_finalOutputSince;
        if ((pending == NI_READY || pending == NI_NO_MEMORY) && elapsed < m_progressTimeout)
        {
            NetExecution::deadline(m_progressTimeout, elapsed);
            if (pending == NI_NO_MEMORY)
            {
                NetExecution::deadline(kFileIoPollMs, 0);
            }
            return NI_NEED_MORE;
        }
        // 文件发布已完成，本地 ACK 无法交付。只结束网络输出，不再提交文件操作。
        m_state->m_failure = pending < 0 && pending != NI_NO_MEMORY ? pending : NI_TIMEOUT;
        m_state->m_phase = NET_FILE_TRANSFER_OUTPUT_FAILED;
        return m_state->m_failure;
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
                m_state->m_batchData = bytes;
                m_state->m_batchOffset = offset;
                m_state->m_batchSize = size;
                prepare(NET_FILE_IO_WRITE, offset, bytes, (std::min)(size, 65536U));
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
        NetExecution::deadline(kFileIoPollMs, 0);
        return NI_NEED_MORE;
    }
    if (submitted == NI_OK)
    {
        NetExecution::progress();
    }
    else
    {
        if (m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
        {
            if (m_state->m_failure == NI_OK) { m_state->m_failure = submitted; }
            NetExecution::deadline(kFileIoPollMs, 0);
        }
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
    // 本轮可能已推进过文件流，结算后让执行者立即处理下一项工作或终态。
    NetExecution::progress();
    if (expected.operation == NET_FILE_IO_CLOSE || expected.operation == NET_FILE_IO_PUBLISH)
    {
        // 已接纳的释放类工作即使失败或取消也消费文件号；不可再次关闭可能复用的号。
        m_state->m_file = 0;
    }
    const bool invalid = result.file != expected.file || result.operation != expected.operation || result.offset != expected.offset ||
        result.result > NI_OK || (result.result == NI_OK && expected.operation == NET_FILE_IO_READ &&
        (result.size != expected.size || result.bytes == nullptr)) ||
        (result.result == NI_OK && expected.operation == NET_FILE_IO_WRITE && result.length != expected.size);
    const int32_t status = invalid ? NI_INVALID_STATE : result.result;
    if (m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
    {
        // 最终ACK只确认远端提交；本地关闭失败仍须报告，已有的首个错误优先。
        if (m_state->m_failure == NI_OK && status < 0)
        {
            m_state->m_failure = status;
        }
        return true;
    }
    if (status != NI_OK)
    {
        cancel(status);
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
        // 每段完成长度已核对，最后一段成功后才确认整批。
        const uint32_t finished = static_cast<uint32_t>(expected.offset - m_state->m_batchOffset) + expected.size;
        if (finished < m_state->m_batchSize)
        {
            prepare(NET_FILE_IO_WRITE, expected.offset + expected.size, m_state->m_batchData + finished,
                (std::min)(m_state->m_batchSize - finished, 65536U));
        }
        else
        {
            m_state->m_batchData = nullptr;
            accepted = m_state->m_flow->written(m_state->m_batchOffset, m_state->m_batchSize, NI_OK);
        }
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
    const int32_t result = m_state->m_flow->acceptForDispatch(frame);
    if (result == NI_OK)
    {
        NetExecution::progress();
    }
    if (result == NI_READY)
    {
        const int32_t reason = m_state->m_flow->progress(m_state->m_now);
        cancel(reason);
        // 对端已终止此流，不能要求宿主重新投递相同的失败 ACK。
        return reason == NI_LIMIT_EXCEEDED ? NI_OK : reason;
    }
    if (result == NI_LIMIT_EXCEEDED)
    {
        // 同一返回码也可来自已终止的协议流。沿用上次 poll 的时间检查，晚到帧无需重试。
        return m_state->m_flow->progress(m_state->m_now) == NI_LIMIT_EXCEEDED ? NI_NOT_FOUND : result;
    }
    if (result < 0 && result != NI_NOT_FOUND)
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
    if (m_state->m_phase == NET_FILE_TRANSFER_OUTPUT_FAILED)
    {
        std::memset(&frame, 0, sizeof(frame));
        return m_state->m_failure;
    }
    return m_state->m_flow->peekOutput(frame);
}

int32_t NetFileTransfer::consumeOutput()
{
    if (m_state != nullptr && m_state->m_phase == NET_FILE_TRANSFER_OUTPUT_FAILED)
    {
        return m_state->m_failure;
    }
    const int32_t result = m_state != nullptr ? m_state->m_flow->consumeOutput() : NI_INVALID_STATE;
    if (result == NI_OK && !done())
    {
        NetExecution::progress();
    }
    return result;
}

void NetFileTransfer::cancel(int32_t reason)
{
    if (done() || m_state->m_phase == NET_FILE_TRANSFER_STOPPING)
    {
        return;
    }
    if (m_state->m_phase == NET_FILE_TRANSFER_DRAIN)
    {
        // 文件已发布。只以最终 ACK 是否已消费结算本地交付，保留协议层的文件状态。
        NetInterfaceFrame output = {};
        const int32_t pending = m_state->m_flow->peekOutput(output);
        if (pending == NI_NEED_MORE)
        {
            m_state->m_phase = NET_FILE_TRANSFER_DONE;
        }
        else
        {
            m_state->m_failure = pending < 0 && pending != NI_NO_MEMORY ? pending :
                (reason < 0 ? reason : NI_CANCELLED);
            m_state->m_phase = NET_FILE_TRANSFER_OUTPUT_FAILED;
        }
        NetExecution::progress();
        return;
    }
    m_state->m_batchData = nullptr;
    // 发送端收到最终 ACK 后仍须收敛其本地文件句柄。
    const int32_t status = m_state->m_flow->progress(m_state->m_now);
    if (status != NI_READY)
    {
        m_state->m_failure = status < 0 ? status : reason < 0 ? reason : NI_CANCELLED;
        m_state->m_flow->cancel(m_state->m_failure);
    }
    m_state->m_phase = NET_FILE_TRANSFER_STOPPING;
    NetExecution::progress();
    m_state->m_waiting = NetFileIoRequest();
    if (m_state->m_job.id != 0)
    {
        m_state->m_io->cancel(m_state->m_job.id);
    }
}

bool NetFileTransfer::done() const
{
    return m_state == nullptr || m_state->m_phase == NET_FILE_TRANSFER_DONE || m_state->m_phase == NET_FILE_TRANSFER_OUTPUT_FAILED;
}

void NetFileTransfer::snapshot(NetInterfaceFileInfo& info, int32_t& result) const
{
    std::memset(&info, 0, sizeof(info));
    result = NI_INVALID_STATE;
    if (m_state != nullptr)
    {
        m_state->m_flow->snapshot(info, result);
        if (m_state->m_failure < 0)
        {
            result = m_state->m_failure;
        }
    }
}