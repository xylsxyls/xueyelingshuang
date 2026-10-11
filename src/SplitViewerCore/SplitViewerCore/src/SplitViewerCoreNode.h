#pragma once
#include "SplitViewerCoreMacro.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreTypes.h"
#include "SplitViewerCoreLeafState.h"
#include <string>
#include <vector>

/** 独占子树及叶子内容，保存创建时的实例配置；原生窗口归Qt层
*/
class SplitViewerCoreAPI SplitViewerCoreNode
{
public:
    // 叶子或分割节点类型
    SplitViewerCoreNodeKind kind;
    // 分割方向
    SplitViewerCoreSplitDirection direction;
    // 第一子区域在父区域中的比例
    double ratio;
    // 独占的第一个子节点，叶子为空
    SplitViewerCoreNode* first;
    // 独占的第二个子节点，叶子为空
    SplitViewerCoreNode* second;
    // 当前叶子的内容和变换状态
    SplitViewerCoreLeafState view;

public:
    /** 初始化对象及其默认状态
    */
    SplitViewerCoreNode();

    /** 复制已校验的文档配置，不保留调用方引用
    @param [in] config 调用方在创建前通过isValid核验的配置
    */
    explicit SplitViewerCoreNode(const SplitViewerCoreConfig& config);

    /** 释放本对象持有的资源
    */
    ~SplitViewerCoreNode();

    /** 查询节点是否为叶节点
    @return 当前为叶节点时返回true
    */
    bool isLeaf() const;

    /** 将叶节点分屏，并将原内容转移到第一个子节点
    @param [in] splitDirection 分屏方向
    */
    void makeSplit(SplitViewerCoreSplitDirection splitDirection);

    /** 按先序收集叶节点；返回的指针由文档持有
    @param [in] node 文档持有的节点
    @param [in,out] leaves 叶节点输出集合
    */
    static void collectLeaves(SplitViewerCoreNode* node, std::vector<SplitViewerCoreNode*>& leaves);

    /** 查询是否包含内容
    @param [in] node 文档持有的节点
    @return 内容类型或图片资源有效时返回true
    */
    static bool hasContent(const SplitViewerCoreNode* node);

    /** 借用对象的只读配置快照
    @return 配置引用，与对象同寿命
    */
    const SplitViewerCoreConfig& config() const;

private:
    // 对象创建时复制的配置，不在运行期修改
    SplitViewerCoreConfig m_config;

private:
    /** 禁止复制对象及其独占资源
    @param [in] other 不可复制的源对象
    */
    SplitViewerCoreNode(const SplitViewerCoreNode& other);

    /** 禁止复制所有权
    @param [in] other 不可复制的源对象
    @return 不提供实现
    */
    SplitViewerCoreNode& operator=(const SplitViewerCoreNode& other);
};