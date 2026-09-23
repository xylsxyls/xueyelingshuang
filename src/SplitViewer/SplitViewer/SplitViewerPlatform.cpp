#include "SplitViewerPlatform.h"
#include <QtGui/QImage>
#include "SplitViewerForeignWindowHelper.h"
#include "SplitViewerFileAssociationHelper.h"
#include "SplitViewerNativeMouseHelper.h"

QWidget* SplitViewerEmbedForeignWindow(WId windowId, QWidget* parent)
{
    return SplitViewerForeignWindowHelper::embedForeignWindow(windowId, parent);
}

void SplitViewerSyncForeignWindow(QWidget* container, const QRect& geometry)
{
    SplitViewerForeignWindowHelper::syncForeignWindow(container, geometry);
}

void SplitViewerDetachForeignWindow(QWidget* container)
{
    SplitViewerForeignWindowHelper::detachForeignWindow(container);
}

WId SplitViewerWindowUnderCursor(WId ownWindowId)
{
    return SplitViewerForeignWindowHelper::windowUnderCursor(ownWindowId);
}

QImage SplitViewerForeignWindowSnapshot(QWidget* container)
{
    return SplitViewerForeignWindowHelper::foreignWindowSnapshot(container);
}

bool SplitViewerIsExternalWindow(WId id,WId own)
{
    return SplitViewerForeignWindowHelper::isExternalWindow(id, own);
}

bool SplitViewerForeignWindowValid(QWidget* container)
{
    return SplitViewerForeignWindowHelper::foreignWindowValid(container);
}

void SplitViewerMoveDetachedWindow(WId id,const QPoint& center)
{
    SplitViewerForeignWindowHelper::moveDetachedWindow(id, center);
}

bool SplitViewerRegisterSvFileAssociation(const QString& applicationPath)
{
    return SplitViewerFileAssociationHelper::registerSvFileAssociation(applicationPath);
}

bool SplitViewerRegisterSvThumbnailHandler(const QString& applicationPath)
{
    return SplitViewerFileAssociationHelper::registerSvThumbnailHandler(applicationPath);
}

void SplitViewerWatchNativeMouse(QObject* owner,const std::function<void(int,const QPoint&,WId)>& callback)
{
    SplitViewerNativeMouseHelper::watchNativeMouse(owner, callback);
}

void SplitViewerUnwatchNativeMouse(QObject* owner)
{
    SplitViewerNativeMouseHelper::unwatchNativeMouse(owner);
}