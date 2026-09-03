#pragma once

#include "Node.h"

/** 分割线命中结果，保存命中的分割节点、方向和拖动计算区域
*/
struct SplitViewerSplitterHit
{
public:
    // 命中图层索引，-1表示底部基础图层
    int layerIndex;
    // 命中节点所属分屏树根节点，不负责释放
    SplitViewerNode* root;
    // 命中的分割节点，不负责释放
    SplitViewerNode* splitNode;
    // 命中分割线方向
    SplitViewerSplitDirection direction;
    // 命中内容所属外层区域
    RECT ownerRect;
    // 命中分屏树根节点区域
    RECT rootRect;
    // 命中分割节点区域
    RECT nodeRect;

public:
    /** 构造空分割线命中结果
    */
    SplitViewerSplitterHit();
};