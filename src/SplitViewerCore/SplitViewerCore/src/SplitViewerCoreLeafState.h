#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>
#include "SplitViewerCoreTypes.h"

struct SplitViewerCoreAPI SplitViewerCoreLeafState
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    std::wstring path;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    SplitViewerCoreContentKind contentKind;
    bool hasImage;
    bool autoFit;
    double scale;
    double offsetX;
    double offsetY;

    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreLeafState();
    /** 清除内容标记并恢复默认图片变换。
    */
    void clear();
    /** 查询是否包含内容。
    @return 是否满足条件或操作成功。
    */
    bool hasContent() const;
};