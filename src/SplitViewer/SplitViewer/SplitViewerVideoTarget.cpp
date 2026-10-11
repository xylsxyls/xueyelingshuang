#include "SplitViewerVideoTarget.h"

SplitViewerVideoTarget::SplitViewerVideoTarget() :
m_node(nullptr)
{

}

SplitViewerVideoTarget::SplitViewerVideoTarget(SplitViewerCoreNode* target, const QString& mediaPath) :
m_node(target),
m_path(mediaPath)
{

}