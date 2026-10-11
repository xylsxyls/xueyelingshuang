#pragma once
#include "SplitViewerCoreMacro.h"
#include "SplitViewerCoreConfig.h"
#include "SplitViewerCoreRect.h"
#include "SplitViewerCoreNode.h"

/** 独占浮动图层的分屏树，归一化矩形相对于舞台；不可复制所有权
*/
struct SplitViewerCoreAPI SplitViewerCoreLayer
{
public:
    // 整层相对舞台的归一化边界
    SplitViewerCoreRect rect;
    // 图层独占的分屏树根
    SplitViewerCoreNode* root;

public:
    /** 初始化对象及其默认状态
    */
    SplitViewerCoreLayer();

    /** 复制已校验的文档配置，不保留调用方引用
    @param [in] config 调用方在创建前通过isValid核验的配置
    */
    explicit SplitViewerCoreLayer(const SplitViewerCoreConfig& config);

    /** 释放本对象持有的资源
    */
    ~SplitViewerCoreLayer();

private:
    /** 禁止复制对象及其独占资源
    @param [in] other 不可复制的源对象
    */
    SplitViewerCoreLayer(const SplitViewerCoreLayer& other);

    /** 禁止复制所有权
    @param [in] other 不可复制的源对象
    @return 不提供实现
    */
    SplitViewerCoreLayer& operator=(const SplitViewerCoreLayer& other);
};