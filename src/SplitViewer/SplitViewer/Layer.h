#pragma once

#include "FloatRect.h"
#include "Node.h"

/** 浮动图层，保存相对舞台区域和独立分屏树根节点
*/
struct SplitViewerLayer
{
public:
    // 图层在底部舞台中的相对位置和大小
    SplitViewerFloatRect rect;
    // 图层内部分屏树根节点，由当前图层负责释放
    SplitViewerNode* root;

public:
    /** 构造默认居中的浮动图层
    */
    SplitViewerLayer();

    /** 析构时释放图层分屏树
    */
    ~SplitViewerLayer();

private:
    /** 禁止拷贝构造，避免多个图层重复管理同一分屏树
    @param [in] other 另一个浮动图层
    */
    SplitViewerLayer(const SplitViewerLayer& other);

    /** 禁止赋值，避免分屏树所有权被复制
    @param [in] other 另一个浮动图层
    @return 返回当前对象引用
    */
    SplitViewerLayer& operator=(const SplitViewerLayer& other);
};