#include "SemaphoreResult.h"

SemaphoreResult::SemaphoreResult(SemaphoreStatus value, bool published, int32_t error) :
status(value),
committed(published),
nativeError(error)
{

}

SemaphoreSignalWaitResult::SemaphoreSignalWaitResult(
    SemaphoreStatus value, SemaphoreCommitState signal, SemaphoreCommitState wait, int32_t error) :
status(value),
signalState(signal),
waitState(wait),
nativeError(error)
{

}