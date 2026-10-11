#pragma once
#include "SplitViewerCoreMacro.h"
#include "SplitViewerCoreResult.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreLayer.h"
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

/** 独占基础树和浮动图层的纯C++文档，配置按实例复制；调用方负责线程亲和性
*/
class SplitViewerCoreAPI SplitViewerCoreDocument
{
public:
    /** 初始化对象及其默认状态
    */
    SplitViewerCoreDocument();

    /** 复制已校验的文档配置，不保留调用方引用
    @param [in] config 调用方在创建前通过isValid核验的配置
    */
    explicit SplitViewerCoreDocument(const SplitViewerCoreConfig& config);

    /** 为未初始化文档复制有效配置，运行期改变须先uninit
    @param [in] config 有限有效的布局和读取预算
    @return 成功复制配置并创建空树；失败保留当前文档
    */
    bool init(const SplitViewerCoreConfig& config);

    /** 释放树和图层，之后须init或reset才可添加内容
    */
    void uninit();

    /** 释放本对象持有的资源
    */
    ~SplitViewerCoreDocument();

    /** 借用背景板根节点，树修改后须重新获取
    @return 借用背景板根节点，树修改后须重新获取
    */
    SplitViewerCoreNode* baseRoot() const;

    /** 返回浮动图层数量
    @return 返回浮动图层数量
    */
    int32_t layerCount() const;

    /** 借用指定图层，越界返回空
    @param [in] index 浮动图层的零基索引，须在当前图层数量范围内
    @return 借用指定图层，越界返回空
    */
    SplitViewerCoreLayer* layerAt(int32_t index) const;

    /** 返回选中图层索引，-1表示背景板
    @return 返回选中图层索引，-1表示背景板
    */
    int32_t selectedLayer() const;

    /** 选择有效图层，索引无效时选中背景板
    @param [in] index 浮动图层的零基索引，须在当前图层数量范围内
    */
    void setSelectedLayer(int32_t index);

    /** 查询边框是否可见
    @return 边框显示时返回true
    */
    bool borderVisible() const;

    /** 设置边框可见性
    @param [in] visible 是否可见
    */
    void setBorderVisible(bool visible);

    /** 返回背景板宽高比
    @return 返回背景板宽高比
    */
    double stageAspect() const;

    /** 设置有效的背景板宽高比
    @param [in] aspect 背景板宽高比
    */
    void setStageAspect(double aspect);

    /** 返回保存的窗口左边界
    @return 返回保存的窗口左边界
    */
    int32_t windowLeft() const;

    /** 返回保存的窗口上边界
    @return 返回保存的窗口上边界
    */
    int32_t windowTop() const;

    /** 返回保存的窗口右边界
    @return 返回保存的窗口右边界
    */
    int32_t windowRight() const;

    /** 返回保存的窗口下边界
    @return 返回保存的窗口下边界
    */
    int32_t windowBottom() const;

    /** 保存窗口矩形坐标供配置恢复
    @param [in] left 左边界
    @param [in] top 上边界
    @param [in] right 右边界
    @param [in] bottom 下边界
    */
    void setWindowRect(int32_t left, int32_t top, int32_t right, int32_t bottom);

    /** 创建并持有一个浮动图层，同时将其选中
    @return 创建并持有一个浮动图层，同时将其选中
    */
    SplitViewerCoreLayer* addLayer();

    /** 删除指定浮动图层并释放其整棵分屏树
    @param [in] index 浮动图层索引，基础层使用-1且不会被删除
    @return 是否找到并删除图层
    */
    bool deleteLayer(int32_t index);

    /** 接收独立根节点的所有权；重复设置当前根节点不作改动
    @param [in] root 树根
    */
    void setBaseRoot(SplitViewerCoreNode* root);

    /** 接收独立图层的所有权；忽略空指针和已持有图层
    @param [in] layer 转移给文档的图层
    */
    void appendLayer(SplitViewerCoreLayer* layer);

    /** 释放所有浮动图层并清除图层选择
    */
    void clearLayers();

    /** 释放全部内容并恢复默认文档状态
    */
    void reset();

    /** 交换两个文档的所有权与显示状态
    @param [in,out] other 另一个对象
    */
    void swap(SplitViewerCoreDocument& other);

    /** 删除目标叶节点并折叠其父节点，未命中返回false
    @param [in] root 树根
    @param [in] target 目标节点
    @return 删除目标叶节点并折叠其父节点，未命中返回false是否成功
    */
    bool deleteLeaf(SplitViewerCoreNode* root, SplitViewerCoreNode* target);

    /** 借用子树的第一个叶节点，空树返回空
    @param [in] root 树根
    @return 借用子树的第一个叶节点，空树返回空
    */
    SplitViewerCoreNode* firstLeaf(SplitViewerCoreNode* root) const;

    /** 返回最近一次受控文档操作的结果，查询函数不改变结果
    @return 明确的成功、未初始化、非法输入或资源预算错误
    */
    SplitViewerCoreResult lastResult() const;

    /** 对本文档的叶子分屏并核验深度及节点预算
    @param [in] node 本文档持有的叶子
    @param [in] direction 水平或垂直方向
    @return 是否实际分屏，失败原因由lastResult取得
    */
    bool splitNode(SplitViewerCoreNode* node, SplitViewerCoreSplitDirection direction);

    /** 捕获场景快照，不共享任何树或字符串可变状态
    @param [out] destination 不同于本对象的独立文档
    @param [out] nodes 源节点到副本节点映射，失败保持原值
    @return 成功提交完整快照；调用方保证复制期间源文档不变
    */
    bool copyTo(
        SplitViewerCoreDocument& destination, std::map<const SplitViewerCoreNode*, SplitViewerCoreNode*>& nodes) const;

    /** 借用对象的只读配置快照
    @return 配置引用，与对象同寿命
    */
    const SplitViewerCoreConfig& config() const;

private:
    // 受控操作的最近结果，仅所属线程修改
    SplitViewerCoreResult m_lastResult;
    // 对象创建时复制的配置，不在运行期修改
    SplitViewerCoreConfig m_config;
    // 文档独占的基础树根
    SplitViewerCoreNode* m_baseRoot;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    // 文档独占的浮动层集合
    std::vector<SplitViewerCoreLayer*> m_layers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前层索引，-1为基础层
    int32_t m_selectedLayer;
    // 边框显示状态
    bool m_borderVisible;
    // 有限正数的舞台宽高比
    double m_stageAspect;
    // 保存的窗口左边界
    int32_t m_windowLeft;
    // 保存的窗口上边界
    int32_t m_windowTop;
    // 保存的窗口右边界
    int32_t m_windowRight;
    // 保存的窗口下边界
    int32_t m_windowBottom;

private:
    /** 禁止复制对象及其独占资源
    @param [in] other 不可复制的源对象
    */
    SplitViewerCoreDocument(const SplitViewerCoreDocument& other);

    /** 禁止复制所有权
    @param [in] other 不可复制的源对象
    @return 不提供实现
    */
    SplitViewerCoreDocument& operator=(const SplitViewerCoreDocument& other);
};