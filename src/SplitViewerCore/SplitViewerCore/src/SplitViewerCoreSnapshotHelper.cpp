#include "SplitViewerCoreSnapshotHelper.h"
#include <memory>

SplitViewerCoreNode* SplitViewerCoreSnapshotHelper::copyNode(const SplitViewerCoreNode* source, int32_t depth,
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes, int32_t& count,
    const SplitViewerCoreConfig& config)
{
    if (!source || !source->config().isValid() || depth > config.m_maximumTreeDepth || count >= config.m_maximumNodes)
    {
        return nullptr;
    }
    ++count;
    std::unique_ptr<SplitViewerCoreNode> node(new SplitViewerCoreNode(source->config()));
    node->kind = source->kind;
    node->direction = source->direction;
    node->ratio = source->ratio;
    node->view = source->view;
    if (!source->isLeaf())
    {
        node->first = copyNode(source->first, depth + 1, nodes, count, config);
        node->second = copyNode(source->second, depth + 1, nodes, count, config);
        if (!node->first || !node->second)
        {
            return nullptr;
        }
    }
    nodes[source] = node.get();
    return node.release();
}

bool SplitViewerCoreSnapshotHelper::copy(const SplitViewerCoreDocument& source, SplitViewerCoreDocument& destination,
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes)
{
    if (&source == &destination || !source.baseRoot() || !source.config().isValid() ||
        source.layerCount() > source.config().m_maximumLayers)
    {
        return false;
    }
    SplitViewerCoreDocument copy(source.config());
    std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*> mappings;
    int32_t count = 0;
    std::unique_ptr<SplitViewerCoreNode> base(copyNode(source.baseRoot(), 0, mappings, count, source.config()));
    if (!base)
    {
        return false;
    }
    copy.setBaseRoot(base.release());
    for (int32_t index = 0; index < source.layerCount(); ++index)
    {
        const SplitViewerCoreLayer* sourceLayer = source.layerAt(index);
        std::unique_ptr<SplitViewerCoreLayer> layer(new SplitViewerCoreLayer(source.config()));
        count = 0;
        std::unique_ptr<SplitViewerCoreNode> tree(copyNode(sourceLayer->root, 0, mappings, count, source.config()));
        if (!tree)
        {
            return false;
        }
        delete layer->root;
        layer->root = tree.release();
        layer->rect = sourceLayer->rect;
        copy.appendLayer(layer.get());
        layer.release();
    }
    copy.setSelectedLayer(source.selectedLayer());
    copy.setBorderVisible(source.borderVisible());
    copy.setStageAspect(source.stageAspect());
    copy.setWindowRect(source.windowLeft(), source.windowTop(), source.windowRight(), source.windowBottom());
    destination.swap(copy);
    nodes.swap(mappings);
    return true;
}

bool SplitViewerCoreSnapshotHelper::locate(
    SplitViewerCoreNode* root, SplitViewerCoreNode* target, int32_t depth, int32_t& targetDepth, int32_t& count)
{
    if (!root || depth > root->config().m_maximumTreeDepth)
    {
        return false;
    }
    ++count;
    const bool found = root == target;
    if (found)
    {
        targetDepth = depth;
    }
    const bool first = locate(root->first, target, depth + 1, targetDepth, count);
    const bool second = locate(root->second, target, depth + 1, targetDepth, count);
    return found || first || second;
}