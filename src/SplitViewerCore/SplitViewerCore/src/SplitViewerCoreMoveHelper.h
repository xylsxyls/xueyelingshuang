#pragma once
#include "SplitViewerCoreNode.h"
#include <stdint.h>

/** 同一分屏树中的方向选择及内容迁移，无平台或界面依赖
*/
class SplitViewerCoreMoveHelper
{
public:
    /** 查找最近可向指定方向移动的相邻子树的首个叶子
    @param [in] root 当前图层根节点，不能传入其他图层
    @param [in] source 当前叶子，必须属于root
    @param [in] direction 移动方向
    @return 借用的固定目标叶子，没有目标或参数无效时返回空
    */
    static SplitViewerCoreNode* directionalTarget(
        SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreMoveDirection direction);

    /** 移动叶子内容，覆盖目标后将源恢复为空；树结构不变
    @param [in] root 源与目标共同所属的图层根节点
    @param [in,out] source 有内容的源叶子
    @param [in,out] target 目标叶子，覆盖确认由调用方完成
    @return 成功返回true；无效参数返回false且不改变任何内容
    */
    static bool moveContent(SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreNode* target);

private:
    /** 沿包含源的路径选取最近的可移动相邻子树
    @param [in] node 当前子树
    @param [in] source 源叶子
    @param [in] direction 移动方向
    @param [in] depth 当前深度，超过读取预算时拒绝
    @param [out] branch 最近的相邻子树，未找到时保持空
    @return 当前子树是否包含源叶子
    */
    static bool findBranch(SplitViewerCoreNode* node, SplitViewerCoreNode* source,
        SplitViewerCoreMoveDirection direction, int32_t depth, SplitViewerCoreNode*& branch);

    /** 在深度预算内检查指针是否属于此树，不转移所有权
    @param [in] node 当前子树
    @param [in] target 待查叶子
    @param [in] depth 当前深度
    @return 是否包含目标
    */
    static bool contains(SplitViewerCoreNode* node, SplitViewerCoreNode* target, int32_t depth);
};