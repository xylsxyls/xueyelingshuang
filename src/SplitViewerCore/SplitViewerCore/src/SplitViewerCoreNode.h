#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>
#include "SplitViewerCoreTypes.h"
#include "SplitViewerCoreLeafState.h"

class SplitViewerCoreAPI SplitViewerCoreNode
{
public:
    SplitViewerCoreNodeKind kind;
    SplitViewerCoreSplitDirection direction;
    double ratio;
    SplitViewerCoreNode* first;
    SplitViewerCoreNode* second;
    SplitViewerCoreLeafState view;

    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreNode();
    /** 释放本对象持有的资源。
    */
    ~SplitViewerCoreNode();

    /** 查询节点是否为叶节点。
    @return 是否满足条件或操作成功。
    */
    bool isLeaf() const;
    /** 将叶节点分屏，并将原内容转移到第一个子节点。
    @param [in] splitDirection 分屏方向
    */
    void makeSplit(SplitViewerCoreSplitDirection splitDirection);

    /** 按先序收集叶节点；返回的指针由文档持有。
    @param [in] node 文档持有的节点
    @param [in,out] leaves 叶节点输出集合
    */
    static void collectLeaves(SplitViewerCoreNode* node, std::vector<SplitViewerCoreNode*>& leaves);
    /** 查询是否包含内容。
    @param [in] node 文档持有的节点
    @return 是否满足条件或操作成功。
    */
    static bool hasContent(const SplitViewerCoreNode* node);

private:
    /** 初始化对象及其默认状态。
    @param [in] other 另一个对象
    */
    SplitViewerCoreNode(const SplitViewerCoreNode& other);
    SplitViewerCoreNode& operator=(const SplitViewerCoreNode& other);
};