#include "SplitViewerCoreConfig.h"
#include <cmath>

SplitViewerCoreConfig::SplitViewerCoreConfig() :
m_defaultStageAspect(4.0 / 3.0),
m_minimumStageAspect(0.1),
m_defaultSplitRatio(0.5),
m_minimumSplitRatio(0.02),
m_maximumSplitRatio(0.98),
m_layerOrigin(0.22),
m_layerOffset(0.03),
m_layerOffsetPeriod(6),
m_layerExtent(0.46),
m_layerOriginLimit(0.72),
m_defaultLayerStart(0.25),
m_defaultLayerEnd(0.75),
m_profileMinimumLayerWidth(0.12),
m_profileMinimumLayerHeight(0.10),
m_zoomStep(1.05),
m_fineZoomStep(1.01),
m_minimumScale(0.0001),
m_minimumFitScale(0.05),
m_maximumFitScale(50.0),
m_minimumZoomRange(10.0),
m_maximumTreeDepth(128),
m_maximumNodes(4096),
m_maximumLayers(256),
m_maximumProfileBytes(16 * 1024 * 1024)
{

}

bool SplitViewerCoreConfig::isValid() const
{
    if (!std::isfinite(m_defaultStageAspect) || m_defaultStageAspect < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_minimumStageAspect) || m_minimumStageAspect < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_defaultSplitRatio) || m_defaultSplitRatio < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_minimumSplitRatio) || m_minimumSplitRatio < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_maximumSplitRatio) || m_maximumSplitRatio < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_layerOrigin) || m_layerOrigin < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_layerOffset) || m_layerOffset < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_layerExtent) || m_layerExtent < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_layerOriginLimit) || m_layerOriginLimit < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_defaultLayerStart) || m_defaultLayerStart < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_defaultLayerEnd) || m_defaultLayerEnd < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_profileMinimumLayerWidth) || m_profileMinimumLayerWidth < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_profileMinimumLayerHeight) || m_profileMinimumLayerHeight < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_zoomStep) || m_zoomStep < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_fineZoomStep) || m_fineZoomStep < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_minimumScale) || m_minimumScale < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_minimumFitScale) || m_minimumFitScale < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_maximumFitScale) || m_maximumFitScale < 0.0)
    {
        return false;
    }
    if (!std::isfinite(m_minimumZoomRange) || m_minimumZoomRange < 0.0)
    {
        return false;
    }
    return m_minimumStageAspect > 0.0 && m_defaultStageAspect >= m_minimumStageAspect && m_minimumSplitRatio > 0.0 &&
        m_maximumSplitRatio < 1.0 && m_minimumSplitRatio <= m_defaultSplitRatio &&
        m_defaultSplitRatio <= m_maximumSplitRatio && m_layerOffsetPeriod > 0 && m_layerExtent > 0.0 &&
        m_layerExtent <= 1.0 && m_layerOriginLimit <= 1.0 && m_layerOrigin <= m_layerOriginLimit &&
        m_defaultLayerStart < m_defaultLayerEnd && m_defaultLayerEnd <= 1.0 && m_profileMinimumLayerWidth > 0.0 &&
        m_profileMinimumLayerWidth <= 1.0 && m_profileMinimumLayerHeight > 0.0 && m_profileMinimumLayerHeight <= 1.0 &&
        m_zoomStep > 1.0 && m_fineZoomStep > 1.0 && m_minimumScale > 0.0 && m_minimumFitScale > 0.0 &&
        m_maximumFitScale >= m_minimumFitScale && m_minimumZoomRange >= 1.0 && m_maximumTreeDepth > 0 &&
        m_maximumTreeDepth <= 256 && m_maximumNodes > 0 && m_maximumNodes <= 1000000 && m_maximumLayers > 0 &&
        m_maximumLayers <= 1000000 && m_maximumProfileBytes >= 2;
}