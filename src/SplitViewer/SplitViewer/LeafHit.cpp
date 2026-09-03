#include "LeafHit.h"
SplitViewerLeafHit::SplitViewerLeafHit() :
layerIndex(-1),
    root(NULL),
    leaf(NULL)
{
    ZeroMemory(&ownerRect, sizeof(ownerRect));
    ZeroMemory(&rootRect, sizeof(rootRect));
    ZeroMemory(&leafRect, sizeof(leafRect));
}