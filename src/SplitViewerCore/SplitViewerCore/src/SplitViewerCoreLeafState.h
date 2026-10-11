#pragma once
#include "SplitViewerCoreMacro.h"
#include "SplitViewerCoreTypes.h"
#include <string>

/** 可复制的媒体路径与图片变换值模型，不持有图片、窗口或进程资源
*/
struct SplitViewerCoreAPI SplitViewerCoreLeafState
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4251)
#endif
    // 图片或联动视频路径，UTF宽字符串
    std::wstring path;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 持久化内容类型，枚举值沿用旧格式
    SplitViewerCoreContentKind contentKind;
    // 图片资源是否已成功恢复
    bool hasImage;
    // 是否随内容区域自动适配
    bool autoFit;
    // 手动图片缩放倍数，有限正数
    double scale;
    // 内容中心水平偏移，像素
    double offsetX;
    // 内容中心垂直偏移，像素
    double offsetY;

public:
    /** 初始化对象及其默认状态
    */
    SplitViewerCoreLeafState();

    /** 清除内容标记并恢复默认图片变换
    */
    void clear();

    /** 查询是否包含内容
    @return 内容类型或图片资源有效时返回true
    */
    bool hasContent() const;
};