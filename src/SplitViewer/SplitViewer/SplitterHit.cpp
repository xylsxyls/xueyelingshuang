#include "SplitterHit.h"
SplitViewerSplitterHit::SplitViewerSplitterHit() :
layerIndex(-1),
    root(NULL),
    splitNode(NULL),
    direction(SPLITVIEWER_SPLIT_HORIZONTAL)
{
    ZeroMemory(&ownerRect, sizeof(ownerRect));
    ZeroMemory(&rootRect, sizeof(rootRect));
    ZeroMemory(&nodeRect, sizeof(nodeRect));
}