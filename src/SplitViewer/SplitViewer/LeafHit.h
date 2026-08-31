#pragma once

#include "Node.h"

namespace SplitViewer
{
    struct LeafHit
    {
    public:
        int layerIndex;
        Node* root;
        Node* leaf;
        RECT ownerRect;
        RECT rootRect;
        RECT leafRect;

    public:
        LeafHit();
    };
}