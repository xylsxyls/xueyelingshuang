#pragma once
#include "SplitViewerCoreAPI.h"
#include <QtCore/QRectF>

/** 工作区命中结果，节点由Core文档持有，树修改后立即丢弃 */
struct SplitViewerHit
{
    SplitViewerCoreNode* node;
    SplitViewerCoreNode* root;
    int layer;
    QRectF rect;
    QRectF ownerRect;
    QRectF layerRect;
    bool splitter;
    /** 初始化对象及其默认状态。
    */
    SplitViewerHit();
};