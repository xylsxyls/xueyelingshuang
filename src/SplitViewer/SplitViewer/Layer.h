#pragma once

#include "FloatRect.h"
#include "Node.h"

namespace SplitViewer
{
    struct Layer
    {
    public:
        FloatRect rect;
        Node* root;

    public:
        Layer();

        ~Layer();

    private:
        Layer(const Layer& other);
        Layer& operator=(const Layer& other);
    };
}