#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QRectF>

/** 工作区命中结果，节点由Core文档持有，树修改后立即丢弃
*/
struct SplitViewerHit
{
public:
    // 借用的命中节点，树改变后失效
    SplitViewerCoreNode* m_node;
    // 借用的当前图层根节点
    SplitViewerCoreNode* m_root;
    // 浮动层索引，-1为基础层
    int m_layer;
    // 叶子内容的画布像素矩形
    QRectF m_rect;
    // 命中节点父矩形，画布坐标
    QRectF m_ownerRect;
    // 整层矩形，画布坐标
    QRectF m_layerRect;
    // 是否命中分割线
    bool m_splitter;

public:
    /** 初始化对象及其默认状态
    */
    SplitViewerHit();
};