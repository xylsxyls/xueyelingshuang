#include "SplitViewerCoreMoveHelper.h"
#include "SplitViewerCoreConfig.h"
#include <algorithm>

bool SplitViewerCoreMoveHelper::findBranch(SplitViewerCoreNode* node, SplitViewerCoreNode* source,
    SplitViewerCoreMoveDirection direction, int32_t depth, SplitViewerCoreNode*& branch)
{
    if (!node || depth > node->config().m_maximumTreeDepth)
    {
        return false;
    }
    if (node == source)
    {
        return true;
    }
    if (node->isLeaf() || !node->first || !node->second)
    {
        return false;
    }
    const bool inFirst = findBranch(node->first, source, direction, depth + 1, branch);
    if (!inFirst && !findBranch(node->second, source, direction, depth + 1, branch))
    {
        return false;
    }
    const bool forward = direction == SPLITVIEWER_CORE_MOVE_RIGHT || direction == SPLITVIEWER_CORE_MOVE_DOWN;
    const bool vertical = direction == SPLITVIEWER_CORE_MOVE_LEFT || direction == SPLITVIEWER_CORE_MOVE_RIGHT;
    const bool matchingSplit =
        node->direction == (vertical ? SPLITVIEWER_CORE_SPLIT_VERTICAL : SPLITVIEWER_CORE_SPLIT_HORIZONTAL);
    // 递归回溯时只登记首个可跨越的分割，保持与源叶子最近的同层邻接关系
    if (!branch && matchingSplit && inFirst == forward)
    {
        branch = inFirst ? node->second : node->first;
    }
    return true;
}

SplitViewerCoreNode* SplitViewerCoreMoveHelper::directionalTarget(
    SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreMoveDirection direction)
{
    if (!root || !source || direction < SPLITVIEWER_CORE_MOVE_LEFT || direction > SPLITVIEWER_CORE_MOVE_DOWN)
    {
        return nullptr;
    }
    SplitViewerCoreNode* branch = nullptr;
    if (!findBranch(root, source, direction, 0, branch) || !source->isLeaf())
    {
        return nullptr;
    }
    // 目标仍有分屏时固定沿第一子区域下降，对应上方或左方，不能跨到其他图层
    for (int32_t depth = 0; branch && !branch->isLeaf(); ++depth)
    {
        if (depth >= root->config().m_maximumTreeDepth)
        {
            return nullptr;
        }
        branch = branch->first;
    }
    return branch;
}

bool SplitViewerCoreMoveHelper::contains(SplitViewerCoreNode* node, SplitViewerCoreNode* target, int32_t depth)
{
    if (!node || depth > node->config().m_maximumTreeDepth)
    {
        return false;
    }
    return node == target ||
        (!node->isLeaf() && (contains(node->first, target, depth + 1) || contains(node->second, target, depth + 1)));
}

bool SplitViewerCoreMoveHelper::moveContent(
    SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreNode* target)
{
    if (!root || !source || !target || source == target || !contains(root, source, 0) || !contains(root, target, 0) ||
        !source->isLeaf() || !target->isLeaf() || !source->view.hasContent())
    {
        return false;
    }
    source->view.path.swap(target->view.path);
    std::swap(source->view.contentKind, target->view.contentKind);
    std::swap(source->view.hasImage, target->view.hasImage);
    std::swap(source->view.autoFit, target->view.autoFit);
    std::swap(source->view.scale, target->view.scale);
    std::swap(source->view.offsetX, target->view.offsetX);
    std::swap(source->view.offsetY, target->view.offsetY);
    source->view.clear();
    return true;
}