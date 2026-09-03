#include "Layer.h"
SplitViewerLayer::SplitViewerLayer() :
root(new SplitViewerNode())
{
    rect.left = 0.25;
    rect.top = 0.25;
    rect.right = 0.75;
    rect.bottom = 0.75;
}

SplitViewerLayer::~SplitViewerLayer()
{
    delete root;
    root = NULL;
}