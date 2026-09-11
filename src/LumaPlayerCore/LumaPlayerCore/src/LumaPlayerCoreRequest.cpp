#include "LumaPlayerCoreRequest.h"

LumaPlayerCoreRequest::LumaPlayerCoreRequest() :
m_requestId(0),
m_mediaGeneration(0),
m_operation(LumaPlayerCoreOperationPlay),
m_position100ns(0),
m_playAfterSeek(false),
m_ratePermille(1000),
m_point(LumaPlayerCoreLoopPointA),
m_frameOffset(0)
{

}

LumaPlayerCoreCompletion::LumaPlayerCoreCompletion() :
m_result(LumaPlayerCoreResultCanceled)
{

}