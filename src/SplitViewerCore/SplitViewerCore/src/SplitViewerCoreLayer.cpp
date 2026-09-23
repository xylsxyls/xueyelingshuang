#include "SplitViewerCoreLayer.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreLayer::SplitViewerCoreLayer() :
    rect(0.25, 0.25, 0.75, 0.75),
    root(new SplitViewerCoreNode())
{
}

SplitViewerCoreLayer::~SplitViewerCoreLayer()
{
    delete root;
    root = NULL;
}