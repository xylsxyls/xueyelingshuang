#include "SplitViewerLayoutHelper.h"
#include "Config.h"
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <algorithm>
#include <cmath>

bool SplitViewerLayoutHelper::findLeafRect(
    SplitViewerCoreNode* root, const QRectF& rect, SplitViewerCoreNode* target, QRectF& result, bool borderVisible)
{
    if (!root || !target)
    {
        return false;
    }
    if (root->isLeaf())
    {
        if (root == target)
        {
            result = rect;
            return true;
        }
        return false;
    }
    QRectF first, splitter, second;
    nodeRects(rect, root, first, splitter, second, borderVisible);
    return findLeafRect(root->first, first, target, result, borderVisible) ||
        findLeafRect(root->second, second, target, result, borderVisible);
}

QRectF SplitViewerLayoutHelper::rectFromCore(const SplitViewerCoreRect& value)
{
    return QRectF(value.left, value.top, value.width(), value.height());
}

QRectF SplitViewerLayoutHelper::stageRect(const QSize& size, double aspect, bool fullscreen)
{
    const qreal margin = fullscreen ? 0.0 : g_config.m_stageMargin;
    const QRectF available(margin, margin, (std::max)(1, size.width() - static_cast<int>(margin * 2.0)),
        (std::max)(1, size.height() - static_cast<int>(margin * 2.0)));
    if (!std::isfinite(aspect) || aspect <= 0.0)
    {
        aspect = g_config.m_coreConfig.m_defaultStageAspect;
    }
    qreal width = available.width();
    qreal height = width / aspect;
    if (height > available.height())
    {
        height = available.height();
        width = height * aspect;
    }
    return QRectF(available.center().x() - width / 2.0, available.center().y() - height / 2.0, width, height);
}

QRectF SplitViewerLayoutHelper::normalizedToPixel(const SplitViewerCoreRect& rect, const QRectF& stage)
{
    return QRectF(stage.left() + rect.left * stage.width(), stage.top() + rect.top * stage.height(),
        rect.width() * stage.width(), rect.height() * stage.height());
}

QRectF SplitViewerLayoutHelper::plusButtonRect(const QRectF& rect)
{
    const qreal size = (std::min<qreal>)(g_config.m_plusButtonSize,
        (std::min)(rect.width(), rect.height()) - g_config.m_plusButtonPadding);
    if (size <= 0.0)
    {
        return QRectF();
    }
    return QRectF(rect.center().x() - size / 2.0, rect.center().y() - size / 2.0, size, size);
}

QRectF SplitViewerLayoutHelper::contentRect(const QRectF& owner, bool borderVisible)
{
    return borderVisible && owner.width() > g_config.m_borderWidth * 2 && owner.height() > g_config.m_borderWidth * 2
        ? owner.adjusted(
              g_config.m_borderWidth, g_config.m_borderWidth, -g_config.m_borderWidth, -g_config.m_borderWidth)
        : owner;
}

void SplitViewerLayoutHelper::nodeRects(
    const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter, QRectF& second, bool borderVisible)
{
    SplitViewerCoreRect a, line, b;
    SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(owner.left(), owner.top(), owner.right(), owner.bottom()), node,
        a, line, b, borderVisible ? g_config.m_splitterWidth : 0.0);
    first = SplitViewerLayoutHelper::rectFromCore(a);
    splitter = SplitViewerLayoutHelper::rectFromCore(line);
    second = SplitViewerLayoutHelper::rectFromCore(b);
}