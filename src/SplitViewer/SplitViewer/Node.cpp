#include "Node.h"

namespace SplitViewer
{
    Node::Node() :
    kind(NODE_LEAF),
    direction(SPLIT_HORIZONTAL),
    ratio(0.5),
    first(NULL),
    second(NULL)
    {

    }

    Node::~Node()
    {
        delete first;
        delete second;
        first = NULL;
        second = NULL;
    }

    bool Node::IsLeaf() const
    {
        return kind == NODE_LEAF;
    }

    void Node::MakeSplit(SplitDirection splitDirection)
    {
        if (!IsLeaf())
        {
            return;
        }

        Node* oldLeaf = new Node();
        oldLeaf->view.TakeFrom(view);
        Node* newLeaf = new Node();

        kind = NODE_SPLIT;
        direction = splitDirection;
        ratio = 0.5;
        first = oldLeaf;
        second = newLeaf;
    }
}