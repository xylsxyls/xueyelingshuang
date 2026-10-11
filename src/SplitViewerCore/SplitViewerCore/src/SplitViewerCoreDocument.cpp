#include "SplitViewerCoreDocument.h"
#include "SplitViewerCoreSnapshotHelper.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreGeometryHelper.h"
#include <algorithm>
#include <cmath>
#include <memory>

SplitViewerCoreDocument::SplitViewerCoreDocument() :
m_lastResult(SPLITVIEWER_CORE_SUCCESS),
m_baseRoot(new SplitViewerCoreNode(m_config)),
m_selectedLayer(-1),
m_borderVisible(true),
m_stageAspect(m_config.m_defaultStageAspect),
m_windowLeft(-1),
m_windowTop(-1),
m_windowRight(-1),
m_windowBottom(-1)
{

}

SplitViewerCoreDocument::SplitViewerCoreDocument(const SplitViewerCoreConfig& config) :
m_lastResult(config.isValid() ? SPLITVIEWER_CORE_SUCCESS : SPLITVIEWER_CORE_INVALID_PARAM),
m_config(config),
m_baseRoot(config.isValid() ? new SplitViewerCoreNode(m_config) : nullptr),
m_selectedLayer(-1),
m_borderVisible(true),
m_stageAspect(m_config.m_defaultStageAspect),
m_windowLeft(-1),
m_windowTop(-1),
m_windowRight(-1),
m_windowBottom(-1)
{

}

const SplitViewerCoreConfig& SplitViewerCoreDocument::config() const
{
    return m_config;
}

bool SplitViewerCoreDocument::init(const SplitViewerCoreConfig& config)
{
    if (!config.isValid())
    {
        m_lastResult = SPLITVIEWER_CORE_INVALID_PARAM;
        return false;
    }
    if (m_baseRoot)
    {
        m_lastResult = SPLITVIEWER_CORE_ALREADY_INIT;
        return false;
    }
    SplitViewerCoreDocument initialized(config);
    swap(initialized);
    return true;
}

void SplitViewerCoreDocument::uninit()
{
    clearLayers();
    delete m_baseRoot;
    m_baseRoot = nullptr;
    m_lastResult = SPLITVIEWER_CORE_NOT_INIT;
}

SplitViewerCoreDocument::~SplitViewerCoreDocument()
{
    delete m_baseRoot;
    m_baseRoot = nullptr;
    clearLayers();
}

SplitViewerCoreNode* SplitViewerCoreDocument::baseRoot() const
{
    return m_baseRoot;
}

int32_t SplitViewerCoreDocument::layerCount() const
{
    return static_cast<int32_t>(m_layers.size());
}

SplitViewerCoreLayer* SplitViewerCoreDocument::layerAt(int32_t index) const
{
    return index >= 0 && index < static_cast<int32_t>(m_layers.size()) ? m_layers[index] : nullptr;
}

int32_t SplitViewerCoreDocument::selectedLayer() const
{
    return m_selectedLayer;
}

void SplitViewerCoreDocument::setSelectedLayer(int32_t index)
{
    m_selectedLayer = index >= -1 && index < layerCount() ? index : -1;
}

bool SplitViewerCoreDocument::borderVisible() const
{
    return m_borderVisible;
}

void SplitViewerCoreDocument::setBorderVisible(bool visible)
{
    m_borderVisible = visible;
}

double SplitViewerCoreDocument::stageAspect() const
{
    return m_stageAspect;
}

void SplitViewerCoreDocument::setStageAspect(double aspect)
{
    m_stageAspect =
        std::isfinite(aspect) && aspect >= m_config.m_minimumStageAspect ? aspect : m_config.m_defaultStageAspect;
}

int32_t SplitViewerCoreDocument::windowLeft() const
{
    return m_windowLeft;
}

int32_t SplitViewerCoreDocument::windowTop() const
{
    return m_windowTop;
}

int32_t SplitViewerCoreDocument::windowRight() const
{
    return m_windowRight;
}

int32_t SplitViewerCoreDocument::windowBottom() const
{
    return m_windowBottom;
}

void SplitViewerCoreDocument::setWindowRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    m_windowLeft = left;
    m_windowTop = top;
    m_windowRight = right;
    m_windowBottom = bottom;
}

SplitViewerCoreLayer* SplitViewerCoreDocument::addLayer()
{
    if (!m_baseRoot)
    {
        m_lastResult = SPLITVIEWER_CORE_NOT_INIT;
        return nullptr;
    }
    if (layerCount() >= m_config.m_maximumLayers)
    {
        m_lastResult = SPLITVIEWER_CORE_LIMIT_EXCEEDED;
        return nullptr;
    }
    std::unique_ptr<SplitViewerCoreLayer> layer(new SplitViewerCoreLayer(m_config));
    const double offset = m_config.m_layerOffset * static_cast<double>(m_layers.size() % m_config.m_layerOffsetPeriod);
    layer->rect.left =
        SplitViewerCoreGeometryHelper::clampDouble(m_config.m_layerOrigin + offset, 0.0, m_config.m_layerOriginLimit);
    layer->rect.top =
        SplitViewerCoreGeometryHelper::clampDouble(m_config.m_layerOrigin + offset, 0.0, m_config.m_layerOriginLimit);
    layer->rect.right = SplitViewerCoreGeometryHelper::clampDouble(
        layer->rect.left + m_config.m_layerExtent, m_config.m_defaultLayerStart, 1.0);
    layer->rect.bottom = SplitViewerCoreGeometryHelper::clampDouble(
        layer->rect.top + m_config.m_layerExtent, m_config.m_defaultLayerStart, 1.0);
    m_layers.push_back(layer.get());
    m_selectedLayer = layerCount() - 1;
    m_lastResult = SPLITVIEWER_CORE_SUCCESS;
    return layer.release();
}

bool SplitViewerCoreDocument::deleteLayer(int32_t index)
{
    if (index < 0 || index >= layerCount())
    {
        return false;
    }
    delete m_layers[static_cast<size_t>(index)];
    m_layers.erase(m_layers.begin() + index);
    if (m_layers.empty())
    {
        m_selectedLayer = -1;
    }
    else if (m_selectedLayer == index)
    {
        m_selectedLayer = index < layerCount() ? index : layerCount() - 1;
    }
    else if (m_selectedLayer > index)
    {
        --m_selectedLayer;
    }
    return true;
}

void SplitViewerCoreDocument::setBaseRoot(SplitViewerCoreNode* root)
{
    if (root == m_baseRoot)
    {
        return;
    }
    if (!root)
    {
        root = new SplitViewerCoreNode(m_config);
    }
    delete m_baseRoot;
    m_baseRoot = root;
}

void SplitViewerCoreDocument::appendLayer(SplitViewerCoreLayer* layer)
{
    if (!layer || std::find(m_layers.begin(), m_layers.end(), layer) != m_layers.end())
    {
        return;
    }
    m_layers.push_back(layer);
}

void SplitViewerCoreDocument::clearLayers()
{
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        delete m_layers[i];
    }
    m_layers.clear();
    if (m_selectedLayer >= 0)
    {
        m_selectedLayer = -1;
    }
}

void SplitViewerCoreDocument::reset()
{
    if (!m_config.isValid())
    {
        m_lastResult = SPLITVIEWER_CORE_INVALID_PARAM;
        return;
    }
    m_lastResult = SPLITVIEWER_CORE_SUCCESS;
    std::unique_ptr<SplitViewerCoreNode> emptyRoot(new SplitViewerCoreNode(m_config));
    delete m_baseRoot;
    m_baseRoot = emptyRoot.release();
    clearLayers();
    m_selectedLayer = -1;
    m_borderVisible = true;
    m_stageAspect = m_config.m_defaultStageAspect;
    m_windowLeft = -1;
    m_windowTop = -1;
    m_windowRight = -1;
    m_windowBottom = -1;
}

void SplitViewerCoreDocument::swap(SplitViewerCoreDocument& other)
{
    std::swap(m_lastResult, other.m_lastResult);
    std::swap(m_config, other.m_config);
    std::swap(m_baseRoot, other.m_baseRoot);
    m_layers.swap(other.m_layers);
    std::swap(m_selectedLayer, other.m_selectedLayer);
    std::swap(m_borderVisible, other.m_borderVisible);
    std::swap(m_stageAspect, other.m_stageAspect);
    std::swap(m_windowLeft, other.m_windowLeft);
    std::swap(m_windowTop, other.m_windowTop);
    std::swap(m_windowRight, other.m_windowRight);
    std::swap(m_windowBottom, other.m_windowBottom);
}

bool SplitViewerCoreDocument::deleteLeaf(SplitViewerCoreNode* root, SplitViewerCoreNode* target)
{
    if (!root || !target || !target->isLeaf() || root->isLeaf())
    {
        return false;
    }
    if (root->first == target || root->second == target)
    {
        SplitViewerCoreNode* sibling = root->first == target ? root->second : root->first;
        SplitViewerCoreNode* removed = target;
        if (sibling && sibling->isLeaf())
        {
            root->kind = SPLITVIEWER_CORE_NODE_LEAF;
            root->view = sibling->view;
            root->direction = SPLITVIEWER_CORE_SPLIT_HORIZONTAL;
            root->ratio = m_config.m_defaultSplitRatio;
            root->first = nullptr;
            root->second = nullptr;
            delete sibling;
            delete removed;
            return true;
        }
        if (sibling)
        {
            root->kind = SPLITVIEWER_CORE_NODE_SPLIT;
            root->direction = sibling->direction;
            root->ratio = sibling->ratio;
            root->first = sibling->first;
            root->second = sibling->second;
            sibling->first = nullptr;
            sibling->second = nullptr;
            delete sibling;
            delete removed;
            return true;
        }
        return false;
    }
    return deleteLeaf(root->first, target) || deleteLeaf(root->second, target);
}

SplitViewerCoreNode* SplitViewerCoreDocument::firstLeaf(SplitViewerCoreNode* root) const
{
    if (!root)
    {
        return nullptr;
    }
    return root->isLeaf() ? root : firstLeaf(root->first);
}

bool SplitViewerCoreDocument::copyTo(
    SplitViewerCoreDocument& destination, std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes) const
{
    return SplitViewerCoreSnapshotHelper::copy(*this, destination, nodes);
}

SplitViewerCoreResult SplitViewerCoreDocument::lastResult() const
{
    return m_lastResult;
}

bool SplitViewerCoreDocument::splitNode(SplitViewerCoreNode* node, SplitViewerCoreSplitDirection direction)
{
    if (!m_baseRoot)
    {
        m_lastResult = SPLITVIEWER_CORE_NOT_INIT;
        return false;
    }
    if (!node || !node->isLeaf() ||
        (direction != SPLITVIEWER_CORE_SPLIT_HORIZONTAL && direction != SPLITVIEWER_CORE_SPLIT_VERTICAL))
    {
        m_lastResult = SPLITVIEWER_CORE_INVALID_PARAM;
        return false;
    }
    std::vector<SplitViewerCoreNode*> roots;
    roots.push_back(m_baseRoot);
    for (SplitViewerCoreLayer* layer : m_layers)
    {
        roots.push_back(layer->root);
    }
    for (SplitViewerCoreNode* root : roots)
    {
        int32_t depth = 0;
        int32_t count = 0;
        if (!SplitViewerCoreSnapshotHelper::locate(root, node, 0, depth, count))
        {
            continue;
        }
        if (depth >= m_config.m_maximumTreeDepth || count > m_config.m_maximumNodes - 2)
        {
            m_lastResult = SPLITVIEWER_CORE_LIMIT_EXCEEDED;
            return false;
        }
        node->makeSplit(direction);
        m_lastResult = node->isLeaf() ? SPLITVIEWER_CORE_INVALID_PARAM : SPLITVIEWER_CORE_SUCCESS;
        return m_lastResult == SPLITVIEWER_CORE_SUCCESS;
    }
    m_lastResult = SPLITVIEWER_CORE_NOT_FOUND;
    return false;
}