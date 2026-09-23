#include "SplitViewerCoreNode.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreNode::SplitViewerCoreNode() :
    kind(SPLITVIEWER_CORE_NODE_LEAF),
    direction(SPLITVIEWER_CORE_SPLIT_HORIZONTAL),
    ratio(0.5),
    first(NULL),
    second(NULL)
{
}

SplitViewerCoreNode::~SplitViewerCoreNode()
{
    delete first;
    delete second;
    first = NULL;
    second = NULL;
}

bool SplitViewerCoreNode::isLeaf() const { return kind == SPLITVIEWER_CORE_NODE_LEAF; }

void SplitViewerCoreNode::makeSplit(SplitViewerCoreSplitDirection splitDirection)
{
    if (!isLeaf())
    {
        return;
    }
    SplitViewerCoreNode* oldLeaf = new SplitViewerCoreNode();
    oldLeaf->view = view;
    SplitViewerCoreNode* newLeaf = new SplitViewerCoreNode();
    kind = SPLITVIEWER_CORE_NODE_SPLIT;
    direction = splitDirection;
    ratio = 0.5;
    first = oldLeaf;
    second = newLeaf;
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