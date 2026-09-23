#include "SplitViewerCoreLeafState.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreLeafState::SplitViewerCoreLeafState() :
    contentKind(SPLITVIEWER_CORE_CONTENT_EMPTY),
    hasImage(false),
    autoFit(true),
    scale(1.0),
    offsetX(0.0),
    offsetY(0.0)
{
}

void SplitViewerCoreLeafState::clear()
{
    path.clear();
    contentKind = SPLITVIEWER_CORE_CONTENT_EMPTY;
    hasImage = false;
    autoFit = true;
    scale = 1.0;
    offsetX = 0.0;
    offsetY = 0.0;
}

bool SplitViewerCoreLeafState::hasContent() const
{
    return hasImage || contentKind == SPLITVIEWER_CORE_CONTENT_EMBEDDED;
}