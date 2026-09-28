#include "SplitViewerCoreNode.h"
#include "SplitViewerCoreConfig.h"
#include <algorithm>
#include <cmath>
#include <memory>

SplitViewerCoreNode::SplitViewerCoreNode() :
kind(SPLITVIEWER_CORE_NODE_LEAF),
direction(SPLITVIEWER_CORE_SPLIT_HORIZONTAL),
ratio(SplitViewerCoreConfig::kDefaultSplitRatio),
first(nullptr),
second(nullptr)
{

}

SplitViewerCoreNode::~SplitViewerCoreNode()
{
    delete first;
    delete second;
    first = nullptr;
    second = nullptr;
}

bool SplitViewerCoreNode::isLeaf() const
{
    return kind == SPLITVIEWER_CORE_NODE_LEAF;
}

void SplitViewerCoreNode::makeSplit(SplitViewerCoreSplitDirection splitDirection)
{
    if (!isLeaf())
    {
        return;
    }
    std::unique_ptr<SplitViewerCoreNode> oldLeaf(new SplitViewerCoreNode());
    oldLeaf->view = view;
    std::unique_ptr<SplitViewerCoreNode> newLeaf(new SplitViewerCoreNode());
    kind = SPLITVIEWER_CORE_NODE_SPLIT;
    direction = splitDirection;
    ratio = SplitViewerCoreConfig::kDefaultSplitRatio;
    first = oldLeaf.release();
    second = newLeaf.release();
    view.clear();
}

void SplitViewerCoreNode::collectLeaves(SplitViewerCoreNode* node, std::vector<SplitViewerCoreNode*>& leaves)
{
    if (!node)
    {
        return;
    }
    if (node->isLeaf())
    {
        leaves.push_back(node);
        return;
    }
    SplitViewerCoreNode::collectLeaves(node->first, leaves);
    SplitViewerCoreNode::collectLeaves(node->second, leaves);
}

bool SplitViewerCoreNode::hasContent(const SplitViewerCoreNode* node)
{
    if (!node)
    {
        return false;
    }
    if (node->isLeaf())
    {
        return node->view.hasContent();
    }
    return SplitViewerCoreNode::hasContent(node->first) || SplitViewerCoreNode::hasContent(node->second);
}