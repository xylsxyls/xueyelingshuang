#pragma once
#include "SplitViewerPlatform.h"

/** 各平台借用窗口的嵌入、同步和恢复。 */
class SplitViewerForeignWindowHelper
{
public:
    /** 借用外部句柄创建嵌入容器；句柄或宿主无效时返回空。
    @param [in] windowId 借用的外部窗口句柄
    @param [in] parent Qt宿主窗口
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static QWidget* embedForeignWindow(WId windowId, QWidget* parent);
    /** 同步容器及外部窗口客户区尺寸。
    @param [in] container 嵌入容器
    @param [in] geometry 目标几何区域
    */
    static void syncForeignWindow(QWidget* container, const QRect& geometry);
    /** 先恢复外部窗口，再释放Qt容器；不销毁外部进程。
    @param [in] container 嵌入容器
    */
    static void detachForeignWindow(QWidget* container);
    /** 查询鼠标下外部顶层窗口；不支持的平台返回零。
    @param [in] ownWindowId 本进程宿主句柄
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static WId windowUnderCursor(WId ownWindowId);
    /** 通过平台接口获取嵌入窗口的导出快照。
    @param [in] container 嵌入容器
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static QImage foreignWindowSnapshot(QWidget* container);
    /** 判断是否为可嵌入的外部顶层窗口。
    @param [in] id 窗口句柄或节点编号
    @param [in] own 宿主窗口句柄
    @return 是否满足条件或操作成功。
    */
    static bool isExternalWindow(WId id,WId own);
    /** 检查容器借用的窗口是否仍然有效。
    @param [in] container 嵌入容器
    @return 是否满足条件或操作成功。
    */
    static bool foreignWindowValid(QWidget* container);
    /** 将脱离宿主的窗口移至指定中心。
    @param [in] id 窗口句柄或节点编号
    @param [in] center 目标中心坐标
    */
    static void moveDetachedWindow(WId id,const QPoint& center);
};