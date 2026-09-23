#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>
#include "SplitViewerCoreRect.h"
#include "SplitViewerCoreNode.h"

struct SplitViewerCoreAPI SplitViewerCoreLayer
{
    SplitViewerCoreRect rect;
    SplitViewerCoreNode* root;

    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreLayer();
    /** 释放本对象持有的资源。
    */
    ~SplitViewerCoreLayer();

private:
    /** 初始化对象及其默认状态。
    @param [in] other 另一个对象
    */
    SplitViewerCoreLayer(const SplitViewerCoreLayer& other);
    SplitViewerCoreLayer& operator=(const SplitViewerCoreLayer& other);
};