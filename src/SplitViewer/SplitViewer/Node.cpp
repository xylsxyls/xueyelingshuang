#include "Node.h"
SplitViewerNode::SplitViewerNode() :
kind(SPLITVIEWER_NODE_LEAF),
    direction(SPLITVIEWER_SPLIT_HORIZONTAL),
    ratio(0.5),
    first(NULL),
    second(NULL)
{

}

SplitViewerNode::~SplitViewerNode()
{
    delete first;
    delete second;
    first = NULL;
    second = NULL;
}

bool SplitViewerNode::IsLeaf() const
{
    return kind == SPLITVIEWER_NODE_LEAF;
}

void SplitViewerNode::MakeSplit(SplitViewerSplitDirection splitDirection)
{
    if (!IsLeaf())
    {
        return;
    }

    SplitViewerNode* oldLeaf = new SplitViewerNode();
    oldLeaf->view.TakeFrom(view);
    SplitViewerNode* newLeaf = new SplitViewerNode();

    kind = SPLITVIEWER_NODE_SPLIT;
    direction = splitDirection;
    ratio = 0.5;
    first = oldLeaf;
    second = newLeaf;
}