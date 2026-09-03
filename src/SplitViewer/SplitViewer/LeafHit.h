#pragma once

#include "Node.h"

/** 叶子节点命中结果，保存命中的图层、分屏树和屏幕区域信息
*/
struct SplitViewerLeafHit
{
public:
    // 命中图层索引，-1表示底部基础图层
    int layerIndex;
    // 命中节点所属分屏树根节点，不负责释放
    SplitViewerNode* root;
    // 命中的叶子节点，不负责释放
    SplitViewerNode* leaf;
    // 命中内容所属外层区域
    RECT ownerRect;
    // 命中分屏树根节点区域
    RECT rootRect;
    // 命中叶子节点区域
    RECT leafRect;

public:
    /** 构造空命中结果
    */
    SplitViewerLeafHit();
};