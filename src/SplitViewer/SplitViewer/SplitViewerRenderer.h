#pragma once
#include "SplitViewerCoreAPI.h"
#include <QtCore/QMap>
#include <QtCore/QRectF>
#include <QtGui/QImage>
class QPainter;
class QWidget;

/** 单次绘制上下文；借用文档和显示资源，不持有主窗口或处理输入。 */
class SplitViewerRenderer
{
public:
    /** 初始化对象及其默认状态。
    @param [in,out] document 文档模型
    @param [in] images 图片缓存
    @param [in,out] embedded 已登记的嵌入窗口
    @param [in] mouse 鼠标工作区坐标
    @param [in] exporting 是否导出绘制
    @param [in] includeEmbedded 是否导出嵌入窗口快照
    */
    SplitViewerRenderer(SplitViewerCoreDocument& document, const QMap<QString,QImage>& images,
        QMap<SplitViewerCoreNode*,QWidget*>& embedded, const QPoint& mouse, bool exporting, bool includeEmbedded);
    /** 绘制背景板及浮动图层，并裁剪各自的内容。
    @param [in,out] painter 绘图目标
    @param [in] stage 背景板像素矩形
    */
    void drawStage(QPainter& painter, const QRectF& stage);
private:
    /** 递归绘制分屏节点。
    @param [in,out] painter 绘图目标
    @param [in] node 文档持有的节点
    @param [in] rect 区域矩形
    @param [in] layerIndex 图层索引
    */
    void drawNode(QPainter& painter, SplitViewerCoreNode* node, const QRectF& rect, int layerIndex);
    /** 绘制图片、嵌入内容或空白加号。
    @param [in,out] painter 绘图目标
    @param [in] node 文档持有的节点
    @param [in] rect 区域矩形
    @param [in] layerIndex 图层索引
    */
    void drawLeaf(QPainter& painter, SplitViewerCoreNode* node, const QRectF& rect, int layerIndex);
    SplitViewerCoreDocument& m_document;
    const QMap<QString,QImage>& m_imageCache;
    QMap<SplitViewerCoreNode*,QWidget*>& m_embedded;
    QPoint m_mouse;
    bool m_exporting;
    bool m_includeEmbedded;
};