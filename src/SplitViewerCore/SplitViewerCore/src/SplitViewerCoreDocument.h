#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>
#include "SplitViewerCoreLayer.h"

class SplitViewerCoreAPI SplitViewerCoreDocument
{
public:
    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreDocument();
    /** 释放本对象持有的资源。
    */
    ~SplitViewerCoreDocument();

    /** 借用背景板根节点，树修改后须重新获取。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    SplitViewerCoreNode* baseRoot() const;
    /** 返回浮动图层数量。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int layerCount() const;
    /** 借用指定图层，越界返回空。
    @param [in] index 图层索引或字节偏移
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    SplitViewerCoreLayer* layerAt(int index) const;
    /** 返回选中图层索引，-1表示背景板。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int selectedLayer() const;
    /** 选择有效图层，索引无效时选中背景板。
    @param [in] index 图层索引或字节偏移
    */
    void setSelectedLayer(int index);
    /** 查询边框是否可见。
    @return 是否满足条件或操作成功。
    */
    bool borderVisible() const;
    /** 设置边框可见性。
    @param [in] visible 是否可见
    */
    void setBorderVisible(bool visible);
    /** 返回背景板宽高比。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    double stageAspect() const;
    /** 设置有效的背景板宽高比。
    @param [in] aspect 背景板宽高比
    */
    void setStageAspect(double aspect);
    /** 返回保存的窗口左边界。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int windowLeft() const;
    /** 返回保存的窗口上边界。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int windowTop() const;
    /** 返回保存的窗口右边界。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int windowRight() const;
    /** 返回保存的窗口下边界。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    int windowBottom() const;
    /** 保存窗口矩形坐标供配置恢复。
    @param [in] left 左边界
    @param [in] top 上边界
    @param [in] right 右边界
    @param [in] bottom 下边界
    */
    void setWindowRect(int left, int top, int right, int bottom);

    /** 创建并持有一个浮动图层，同时将其选中。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    SplitViewerCoreLayer* addLayer();
    /** 接收独立根节点的所有权；重复设置当前根节点不作改动。
    @param [in] root 树根
    */
    void setBaseRoot(SplitViewerCoreNode* root);
    /** 接收独立图层的所有权；忽略空指针和已持有图层。
    @param [in] layer 转移给文档的图层
    */
    void appendLayer(SplitViewerCoreLayer* layer);
    /** 释放所有浮动图层并清除图层选择。
    */
    void clearLayers();
    /** 释放全部内容并恢复默认文档状态。
    */
    void reset();
    /** 交换两个文档的所有权与显示状态。
    @param [in,out] other 另一个对象
    */
    void swap(SplitViewerCoreDocument& other);

    /** 删除目标叶节点并折叠其父节点，未命中返回false。
    @param [in] root 树根
    @param [in] target 目标节点
    @return 是否满足条件或操作成功。
    */
    bool deleteLeaf(SplitViewerCoreNode* root, SplitViewerCoreNode* target);
    /** 借用子树的第一个叶节点，空树返回空。
    @param [in] root 树根
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    SplitViewerCoreNode* firstLeaf(SplitViewerCoreNode* root) const;

private:
    SplitViewerCoreNode* m_baseRoot;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::vector<SplitViewerCoreLayer*> m_layers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    int m_selectedLayer;
    bool m_borderVisible;
    double m_stageAspect;
    int m_windowLeft;
    int m_windowTop;
    int m_windowRight;
    int m_windowBottom;

    /** 初始化对象及其默认状态。
    @param [in] other 另一个对象
    */
    SplitViewerCoreDocument(const SplitViewerCoreDocument& other);
    SplitViewerCoreDocument& operator=(const SplitViewerCoreDocument& other);
};