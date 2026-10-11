#include "SplitViewerCoreLayer.h"
#include "SplitViewerCoreConfig.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreLayer::SplitViewerCoreLayer() :
rect(SplitViewerCoreConfig().m_defaultLayerStart, SplitViewerCoreConfig().m_defaultLayerStart,
    SplitViewerCoreConfig().m_defaultLayerEnd, SplitViewerCoreConfig().m_defaultLayerEnd),
root(new SplitViewerCoreNode())
{

}

SplitViewerCoreLayer::SplitViewerCoreLayer(const SplitViewerCoreConfig& config) :
rect(config.m_defaultLayerStart, config.m_defaultLayerStart, config.m_defaultLayerEnd, config.m_defaultLayerEnd),
root(new SplitViewerCoreNode(config))
{

}

SplitViewerCoreLayer::~SplitViewerCoreLayer()
{
    delete root;
    root = nullptr;
}