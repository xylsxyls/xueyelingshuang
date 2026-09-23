#include "SplitViewerCoreGeometryHelper.h"
#include <algorithm>
#include <cmath>

double SplitViewerCoreGeometryHelper::clampDouble(double value, double low, double high)
{
    if (value < low)
    {
        return low;
    }
    if (value > high)
    {
        return high;
    }
    return value;
}

void SplitViewerCoreGeometryHelper::splitNodeRects(const SplitViewerCoreRect& owner,
    const SplitViewerCoreNode* node,
    SplitViewerCoreRect& first,
    SplitViewerCoreRect& splitter,
    SplitViewerCoreRect& second,
    double thickness)
{
    const double length=node && node->direction == SPLITVIEWER_CORE_SPLIT_VERTICAL ? owner.width() : owner.height();
    thickness=(std::max)(0.0,(std::min)(thickness,length/3.0));
    const double ratio = clampDouble(node ? node->ratio : 0.5, 0.02, 0.98);
    if (node && node->direction == SPLITVIEWER_CORE_SPLIT_VERTICAL)
    {
        const double position = owner.left + (owner.width() - thickness) * ratio;
        splitter = SplitViewerCoreRect(position, owner.top, position + thickness, owner.bottom);
        first = SplitViewerCoreRect(owner.left, owner.top, position, owner.bottom);
        second = SplitViewerCoreRect(position + thickness, owner.top, owner.right, owner.bottom);
    }
    else
    {
        const double position = owner.top + (owner.height() - thickness) * ratio;
        splitter = SplitViewerCoreRect(owner.left, position, owner.right, position + thickness);
        first = SplitViewerCoreRect(owner.left, owner.top, owner.right, position);
        second = SplitViewerCoreRect(owner.left, position + thickness, owner.right, owner.bottom);
    }
}

void SplitViewerCoreGeometryHelper::constrainLayerRect(SplitViewerCoreRect& rect,
    const SplitViewerCoreRect& bounds,
    double minWidth,
    double minHeight)
{
    minWidth = (std::min)(minWidth, bounds.width());
    minHeight = (std::min)(minHeight, bounds.height());
    if (rect.width() < minWidth)
    {
        rect.right = rect.left + minWidth;
    }
    if (rect.height() < minHeight)
    {
        rect.bottom = rect.top + minHeight;
    }
    if (rect.left < bounds.left)
    {
        rect.right += bounds.left - rect.left;
        rect.left = bounds.left;
    }
    if (rect.top < bounds.top)
    {
        rect.bottom += bounds.top - rect.top;
        rect.top = bounds.top;
    }
    if (rect.right > bounds.right)
    {
        rect.left -= rect.right - bounds.right;
        rect.right = bounds.right;
    }
    if (rect.bottom > bounds.bottom)
    {
        rect.top -= rect.bottom - bounds.bottom;
        rect.bottom = bounds.bottom;
    }
    rect.left = clampDouble(rect.left, bounds.left, bounds.right);
    rect.top = clampDouble(rect.top, bounds.top, bounds.bottom);
    rect.right = clampDouble(rect.right, rect.left, bounds.right);
    rect.bottom = clampDouble(rect.bottom, rect.top, bounds.bottom);
}

double SplitViewerCoreGeometryHelper::fitScale(double imageWidth, double imageHeight, double contentWidth, double contentHeight)
{
    if (imageWidth <= 0.0 || imageHeight <= 0.0 || contentWidth <= 0.0 || contentHeight <= 0.0)
    {
        return 1.0;
    }
    return (std::min)(contentWidth / imageWidth, contentHeight / imageHeight);
}

void SplitViewerCoreGeometryHelper::zoom(SplitViewerCoreLeafState& view, double fitScale, int delta, bool fine)
{
    if (!view.hasImage || fitScale<=0 || delta==0) return;
    if (view.autoFit) view.scale=fitScale;
    view.autoFit=false;
    int steps=delta/120;
    if (!steps) steps=delta>0 ? 1 : -1;
    const double minimum=(std::max)(0.0001,fitScale*0.05);
    const double maximum=(std::max)(minimum*10.0,fitScale*50.0);
    view.scale=clampDouble(view.scale*std::pow(fine ? 1.01 : 1.05,steps),minimum,maximum);
}

void SplitViewerCoreGeometryHelper::resizeView(SplitViewerCoreLeafState& view, double oldWidth, double oldHeight, double newWidth, double newHeight)
{
    if (!view.hasImage || oldWidth<=0 || oldHeight<=0 || newWidth<=0 || newHeight<=0) return;
    const double sx=newWidth/oldWidth;
    const double sy=newHeight/oldHeight;
    view.offsetX*=sx;
    view.offsetY*=sy;
    if (!view.autoFit) view.scale*=(sx+sy)*0.5;
}