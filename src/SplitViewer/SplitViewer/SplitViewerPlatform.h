#pragma once
#include <QtCore/QtGlobal>
#include <QtCore/QRect>
#include <QtCore/QPoint>
#include <QtGui/qwindowdefs.h>
#include <functional>

class QWidget;
class QString;
class QImage;
class QObject;

/** GUI线程借用平台窗口句柄创建Qt外部窗口容器，Core不参与
@param [in] windowId 有效的外部顶层窗口句柄，不接管进程所有权
@param [in] parent Qt宿主，不可空；宿主负责容器生命周期
@return 嵌入容器，失败返回空且不改变外部窗口
*/
QWidget* SplitViewerEmbedForeignWindow(WId windowId, QWidget* parent);

/** GUI线程同步Qt容器及原生客户区，通知视频窗口尺寸变化
@param [in] container 已嵌入的容器，可空，借用至本次调用结束
@param [in] geometry 相对于Qt宿主的目标客户区，逻辑像素
*/
void SplitViewerSyncForeignWindow(QWidget* container, const QRect& geometry);

/** GUI线程恢复外部窗口并销毁Qt容器，不终止外部进程
@param [in] container 待释放的容器，可空；调用后不可继续使用
*/
void SplitViewerDetachForeignWindow(QWidget* container);

/** 查询鼠标下可拖入的外部窗口
@param [in] ownWindowId 本应用宿主句柄，用于排除自身
@return 借用的外部句柄，未命中或平台不支持时返回零
*/
WId SplitViewerWindowUnderCursor(WId ownWindowId);

/** 注册.sv打开命令和图标，非Windows安全返回false
@param [in] applicationPath 本程序的绝对可执行文件路径
@return 全部注册步骤成功时返回true
*/
bool SplitViewerRegisterSvFileAssociation(const QString& applicationPath);

/** 注册.sv的PNG预览处理器，非Windows安全返回false
@param [in] applicationPath 本程序的绝对可执行文件路径
@return 注册成功时返回true
*/
bool SplitViewerRegisterSvThumbnailHandler(const QString& applicationPath);

/** GUI线程获取嵌入窗口快照供导出使用
@param [in] container 平台容器，可空，借用至调用结束
@return 窗口快照，失败或平台不可用时为空
*/
QImage SplitViewerForeignWindowSnapshot(QWidget* container);

/** GUI线程订阅平台鼠标通知，1按下、2移动、3抬起
@param [in] owner 接收通知的Qt对象，销毁前须撤销订阅
@param [in] callback GUI通知回调，依次接收动作、屏幕位置及借用句柄
*/
void SplitViewerWatchNativeMouse(QObject* owner, const std::function<void(int, const QPoint&, WId)>& callback);

/** GUI线程撤销所属对象的鼠标订阅，不影响其他接收者
@param [in] owner 原订阅对象，可空
*/
void SplitViewerUnwatchNativeMouse(QObject* owner);

/** GUI线程核验外部窗口身份和可嵌入性
@param [in] id 借用的候选句柄
@param [in] own 当前宿主句柄
@return 候选有效且属于其他进程的可嵌入顶层窗口时为true
*/
bool SplitViewerIsExternalWindow(WId id, WId own);

/** GUI线程核验容器关联的外部窗口是否仍存活
@param [in] container 借用的容器，可空
@return 容器和对应原生窗口有效时为true
*/
bool SplitViewerForeignWindowValid(QWidget* container);

/** GUI线程将已脱离的外部窗口移至屏幕目标中心
@param [in] id 已恢复为独立窗口的借用句柄
@param [in] center 目标屏幕坐标，平台像素
*/
void SplitViewerMoveDetachedWindow(WId id, const QPoint& center);