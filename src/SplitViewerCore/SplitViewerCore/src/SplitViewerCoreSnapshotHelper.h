#pragma once
#include "SplitViewerCoreDocument.h"
#include <map>

/** 深拷贝独占场景，映射仅用作请求提交时的身份关联
*/
class SplitViewerCoreSnapshotHelper
{
public:
    /** 复制完整有效文档，失败保留原输出及映射
    @param [in] source 由调用方保证本次复制期间不被其他线程修改
    @param [out] destination 独立文档，不可与source为同一对象
    @param [out] nodes 原节点与副本节点映射；不转移原节点所有权
    @return 成功复制且提交全部状态
    */
    static bool copy(const SplitViewerCoreDocument& source, SplitViewerCoreDocument& destination,
        std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes);

    /** 查询树内目标并统计全部节点，不依赖相邻实例
    @param [in] root 当前树根
    @param [in] target 目标指针，只比较身份
    @param [in] depth 当前递归深度
    @param [out] targetDepth 命中深度，未命中不改变
    @param [in,out] count 累加节点数
    @return 是否命中
    */
    static bool locate(
        SplitViewerCoreNode* root, SplitViewerCoreNode* target, int32_t depth, int32_t& targetDepth, int32_t& count);

private:
    /** 递归复制受配置深度限制的树
    @param [in] source 借用源节点
    @param [in] depth 当前层数
    @param [in,out] nodes 本次临时映射
    @param [in,out] count 当前树已复制节点数，不能超过文档预算
    @param [in] config 文档拥有者的预算，不能借节点自己的宽松配置绕过
    @return 独占的新节点，失败返回空并由调用方释放已建子树
    */
    static SplitViewerCoreNode* copyNode(const SplitViewerCoreNode* source, int32_t depth,
        std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes, int32_t& count,
        const SplitViewerCoreConfig& config);
};