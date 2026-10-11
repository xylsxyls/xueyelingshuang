#include "SplitViewerTaskContext.h"

SplitViewerTaskRequest::SplitViewerTaskRequest() :
m_id(0),
m_longRunning(false),
m_success(false)
{

}

SplitViewerTaskContext::SplitViewerTaskContext() :
m_logicId(0),
m_executionId(0),
m_exit(false),
m_retired(false)
{

}