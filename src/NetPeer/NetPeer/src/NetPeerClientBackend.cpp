#include "NetPeerMacro.h"
#include "NetPeerClientBackend.h"
#include "NetPeerClientState.h"

int32_t NetPeerClientBackend::bindSubmission(NetPeerClientHandle handle, NetSenderSubmission* submission)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (handle->m_polling || handle->m_callbackDepth != 0)
    {
        return NI_INVALID_STATE;
    }
    return handle->m_dispatch.bindSubmission(submission);
}