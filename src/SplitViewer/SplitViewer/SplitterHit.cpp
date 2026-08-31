#include "SplitterHit.h"

namespace SplitViewer
{
    SplitterHit::SplitterHit() :
    layerIndex(-1),
    root(NULL),
    splitNode(NULL),
    direction(SPLIT_HORIZONTAL)
    {
        ZeroMemory(&ownerRect, sizeof(ownerRect));
        ZeroMemory(&rootRect, sizeof(rootRect));
        ZeroMemory(&nodeRect, sizeof(nodeRect));
    }
}