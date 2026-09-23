#include "SplitViewerCoreDocument.h"
#include <algorithm>
#include "SplitViewerCoreGeometryHelper.h"
#include <cmath>

SplitViewerCoreDocument::SplitViewerCoreDocument() :
    m_baseRoot(new SplitViewerCoreNode()),
    m_selectedLayer(-1),
    m_borderVisible(true),
    m_stageAspect(4.0 / 3.0),
    m_windowLeft(-1),
    m_windowTop(-1),
    m_windowRight(-1),
    m_windowBottom(-1)
{
}

SplitViewerCoreDocument::~SplitViewerCoreDocument()
{
    delete m_baseRoot;
    m_baseRoot = NULL;
    clearLayers();
}

SplitViewerCoreNode* SplitViewerCoreDocument::baseRoot() const { return m_baseRoot; }
int SplitViewerCoreDocument::layerCount() const { return static_cast<int>(m_layers.size()); }
SplitViewerCoreLayer* SplitViewerCoreDocument::layerAt(int index) const
{
    return index >= 0 && index < static_cast<int>(m_layers.size()) ? m_layers[index] : NULL;
}
int SplitViewerCoreDocument::selectedLayer() const { return m_selectedLayer; }
void SplitViewerCoreDocument::setSelectedLayer(int index)
{
    m_selectedLayer = index >= -1 && index < layerCount() ? index : -1;
}
bool SplitViewerCoreDocument::borderVisible() const { return m_borderVisible; }
void SplitViewerCoreDocument::setBorderVisible(bool visible) { m_borderVisible = visible; }
double SplitViewerCoreDocument::stageAspect() const { return m_stageAspect; }
void SplitViewerCoreDocument::setStageAspect(double aspect)
{
    m_stageAspect = aspect >= 0.1 ? aspect : 4.0 / 3.0;
}
int SplitViewerCoreDocument::windowLeft() const { return m_windowLeft; }
int SplitViewerCoreDocument::windowTop() const { return m_windowTop; }
int SplitViewerCoreDocument::windowRight() const { return m_windowRight; }
int SplitViewerCoreDocument::windowBottom() const { return m_windowBottom; }
void SplitViewerCoreDocument::setWindowRect(int left, int top, int right, int bottom)
{
    m_windowLeft = left;
    m_windowTop = top;
    m_windowRight = right;
    m_windowBottom = bottom;
}

SplitViewerCoreLayer* SplitViewerCoreDocument::addLayer()
{
    SplitViewerCoreLayer* layer = new SplitViewerCoreLayer();
    const double offset = 0.03 * static_cast<double>(m_layers.size() % 6);
    layer->rect.left = SplitViewerCoreGeometryHelper::clampDouble(0.22 + offset, 0.0, 0.72);
    layer->rect.top = SplitViewerCoreGeometryHelper::clampDouble(0.22 + offset, 0.0, 0.72);
    layer->rect.right = SplitViewerCoreGeometryHelper::clampDouble(layer->rect.left + 0.46, 0.25, 1.0);
    layer->rect.bottom = SplitViewerCoreGeometryHelper::clampDouble(layer->rect.top + 0.46, 0.25, 1.0);
    m_layers.push_back(layer);
    m_selectedLayer = layerCount() - 1;
    return layer;
}

void SplitViewerCoreDocument::setBaseRoot(SplitViewerCoreNode* root)
{
    if (root == m_baseRoot)
    {
        return;
    }
    if (!root)
    {
        root = new SplitViewerCoreNode();
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
    delete m_baseRoot;
    m_baseRoot = new SplitViewerCoreNode();
    clearLayers();
    m_selectedLayer = -1;
    m_borderVisible = true;
    m_stageAspect = 4.0 / 3.0;
    m_windowLeft = -1;
    m_windowTop = -1;
    m_windowRight = -1;
    m_windowBottom = -1;
}

void SplitViewerCoreDocument::swap(SplitViewerCoreDocument& other)
{
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
    if (!root || !target || root->isLeaf())
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
            root->ratio = 0.5;
            root->first = NULL;
            root->second = NULL;
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
            sibling->first = NULL;
            sibling->second = NULL;
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
        return NULL;
    }
    return root->isLeaf() ? root : firstLeaf(root->first);
}