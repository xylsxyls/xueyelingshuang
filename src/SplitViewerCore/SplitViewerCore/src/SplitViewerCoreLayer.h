#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>
#include "SplitViewerCoreRect.h"
#include "SplitViewerCoreNode.h"

struct SplitViewerCoreAPI SplitViewerCoreLayer
{
public:
    SplitViewerCoreRect rect;
    SplitViewerCoreNode* root;

    /** 初始化对象及其默认状态
    */
    SplitViewerCoreLayer();

    /** 释放本对象持有的资源
    */
    ~SplitViewerCoreLayer();

private:
    /** 初始化对象及其默认状态
    @param [in] other 另一个对象
    */
    SplitViewerCoreLayer(const SplitViewerCoreLayer& other);
    /** 禁止复制所有权
    @param [in] other 不可复制的源对象
    @return 不提供实现
    */
    SplitViewerCoreLayer& operator=(const SplitViewerCoreLayer& other);
};