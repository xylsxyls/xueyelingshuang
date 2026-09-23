#pragma once

#include <QtCore/QtGlobal>
#include <QtCore/QRect>
#include <QtCore/QPoint>
#include <QtGui/qwindowdefs.h>

class QWidget;
class QString;
class QImage;

/** 使用平台窗口句柄创建Qt外部窗口容器。Core不参与本接口。 */
QWidget* SplitViewerEmbedForeignWindow(WId windowId, QWidget* parent);
/** 同步Qt容器及其平台原生子窗口的实际客户区，确保嵌入视频收到尺寸变化。 */
void SplitViewerSyncForeignWindow(QWidget* container, const QRect& geometry);
void SplitViewerDetachForeignWindow(QWidget* container);
WId SplitViewerWindowUnderCursor(WId ownWindowId);

/** 注册.sv文件关联。缩略图注册只在Windows实现，其他平台保留可编译空实现。 */
bool SplitViewerRegisterSvFileAssociation(const QString& applicationPath);
bool SplitViewerRegisterSvThumbnailHandler(const QString& applicationPath);

/** 获取嵌入窗口快照供导出使用
@param [in] container 平台容器
@return 窗口快照，失败时为空
*/
QImage SplitViewerForeignWindowSnapshot(QWidget* container);

#include <functional>
class QObject;
/** GUI线程接收平台鼠标通知，1按下/2移动/3抬起；句柄只存在Qt层。 */
void SplitViewerWatchNativeMouse(QObject* owner, const std::function<void(int,const QPoint&,WId)>& callback);
void SplitViewerUnwatchNativeMouse(QObject* owner);
bool SplitViewerIsExternalWindow(WId id, WId own);
bool SplitViewerForeignWindowValid(QWidget* container);
void SplitViewerMoveDetachedWindow(WId id, const QPoint& center);