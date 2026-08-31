#pragma once

#include "Node.h"

namespace SplitViewer
{
    struct SplitterHit
    {
    public:
        int layerIndex;
        Node* root;
        Node* splitNode;
        SplitDirection direction;
        RECT ownerRect;
        RECT rootRect;
        RECT nodeRect;

    public:
        SplitterHit();
    };
}