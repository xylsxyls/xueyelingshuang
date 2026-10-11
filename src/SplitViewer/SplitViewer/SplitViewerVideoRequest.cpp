#include "SplitViewerVideoRequest.h"

SplitViewerVideoRequest::SplitViewerVideoRequest() :
m_id(0),
m_processId(0),
m_processToken(0),
m_windowId(0),
m_deadline(0),
m_phase(SPLITVIEWER_VIDEO_QUEUED),
m_forced(false)
{

}