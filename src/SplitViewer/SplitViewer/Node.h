#pragma once

#include "ImageView.h"

/** 分屏树节点，叶子节点保存图片视图，分割节点保存方向、比例和两个子节点生命周期
*/
struct SplitViewerNode
{
public:
    // 节点类型，决定当前节点是叶子还是分割容器
    SplitViewerNodeKind kind;
    // 分割方向，仅分割节点使用
    SplitViewerSplitDirection direction;
    // 第一块区域占父区域的比例
    double ratio;
    // 第一子节点，由当前节点负责释放
    SplitViewerNode* first;
    // 第二子节点，由当前节点负责释放
    SplitViewerNode* second;
    // 叶子节点的图片或嵌入窗口视图
    SplitViewerImageView view;

public:
    /** 构造默认叶子节点
    */
    SplitViewerNode();

    /** 析构时释放子节点
    */
    ~SplitViewerNode();

    /** 判断当前节点是否为叶子节点
    @return 返回true表示当前节点可直接展示内容
    */
    bool IsLeaf() const;

    /** 将当前叶子节点转换为分割节点，原视图内容移动到第一子节点
    @param [in] splitDirection 分割方向
    */
    void MakeSplit(SplitViewerSplitDirection splitDirection);

private:
    /** 禁止拷贝构造，避免分屏树节点所有权被复制
    @param [in] other 另一个分屏树节点
    */
    SplitViewerNode(const SplitViewerNode& other);

    /** 禁止赋值，避免子节点和视图资源所有权被复制
    @param [in] other 另一个分屏树节点
    @return 返回当前对象引用
    */
    SplitViewerNode& operator=(const SplitViewerNode& other);
};