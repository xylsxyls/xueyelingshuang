#include "SplitViewerCoreRect.h"
#include <algorithm>
#include <cmath>

SplitViewerCoreRect::SplitViewerCoreRect() : left(0.0), top(0.0), right(1.0), bottom(1.0)
{
}

SplitViewerCoreRect::SplitViewerCoreRect(double leftValue, double topValue, double rightValue, double bottomValue) :
    left(leftValue), top(topValue), right(rightValue), bottom(bottomValue)
{
}

double SplitViewerCoreRect::width() const { return right - left; }
double SplitViewerCoreRect::height() const { return bottom - top; }