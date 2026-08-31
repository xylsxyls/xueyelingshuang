#include "Layer.h"

namespace SplitViewer
{
    Layer::Layer() :
    root(new Node())
    {
        rect.left = 0.25;
        rect.top = 0.25;
        rect.right = 0.75;
        rect.bottom = 0.75;
    }

    Layer::~Layer()
    {
        delete root;
        root = NULL;
    }
}