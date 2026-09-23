#include "SplitViewerLayoutHelper.h"
#include "SplitViewerCoreAPI.h"
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <algorithm>

QRectF SplitViewerLayoutHelper::rectFromCore(const SplitViewerCoreRect& value)
{
    return QRectF(value.left, value.top, value.width(), value.height());
}

QRectF SplitViewerLayoutHelper::stageRect(const QSize& size, double aspect, bool fullscreen)
{
    const qreal margin = fullscreen ? 0.0 : 10.0;
    const QRectF available(margin, margin, (std::max)(1, size.width() - static_cast<int>(margin * 2.0)),
        (std::max)(1, size.height() - static_cast<int>(margin * 2.0)));
    aspect = aspect > 0.1 ? aspect : 4.0 / 3.0;
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
    const qreal size = (std::min<qreal>)(42.0, (std::min)(rect.width(), rect.height()) - 8.0);
    if (size <= 0.0)
    {
        return QRectF();
    }
    return QRectF(rect.center().x() - size / 2.0, rect.center().y() - size / 2.0, size, size);
}

QRectF SplitViewerLayoutHelper::contentRect(const QRectF& owner, bool borderVisible)
{
    return borderVisible && owner.width() > 2 && owner.height() > 2 ? owner.adjusted(1,1,-1,-1) : owner;
}

void SplitViewerLayoutHelper::nodeRects(const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter, QRectF& second, bool borderVisible)
{
    SplitViewerCoreRect a, line, b;
    SplitViewerCoreSplitNodeRects(SplitViewerCoreRect(owner.left(), owner.top(), owner.right(), owner.bottom()),
        node, a, line, b, borderVisible ? 2.0 : 0.0);
    first = SplitViewerLayoutHelper::rectFromCore(a);
    splitter = SplitViewerLayoutHelper::rectFromCore(line);
    second = SplitViewerLayoutHelper::rectFromCore(b);
}