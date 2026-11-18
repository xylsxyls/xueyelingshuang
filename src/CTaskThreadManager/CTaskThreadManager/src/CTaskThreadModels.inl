#pragma once

inline CTaskThreadResult::CTaskThreadResult(CTaskThreadStatus status, bool committed, int32_t nativeError) :
m_status(status),
m_committed(committed),
m_nativeError(nativeError)
{

}