#pragma once

#include "SplitViewerCommon.h"

namespace SplitViewer
{
    struct FloatRect
    {
    public:
        double left;
        double top;
        double right;
        double bottom;

    public:
        FloatRect();
    };
}