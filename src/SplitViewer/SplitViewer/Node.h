#pragma once

#include "ImageView.h"

namespace SplitViewer
{
    struct Node
    {
    public:
        NodeKind kind;
        SplitDirection direction;
        double ratio;
        Node* first;
        Node* second;
        ImageView view;

    public:
        Node();

        ~Node();

        bool IsLeaf() const;

        void MakeSplit(SplitDirection splitDirection);

    private:
        Node(const Node& other);
        Node& operator=(const Node& other);
    };
}