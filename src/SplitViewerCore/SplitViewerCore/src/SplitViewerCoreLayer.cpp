#include "SplitViewerCoreLayer.h"
#include "SplitViewerCoreConfig.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreLayer::SplitViewerCoreLayer() :
rect(SplitViewerCoreConfig::kDefaultLayerStart, SplitViewerCoreConfig::kDefaultLayerStart, SplitViewerCoreConfig::kDefaultLayerEnd, SplitViewerCoreConfig::kDefaultLayerEnd),
root(new SplitViewerCoreNode())
{

}

SplitViewerCoreLayer::~SplitViewerCoreLayer()
{
    delete root;
    root = nullptr;
}