#pragma once
#include <stdint.h>
/** 核心默认值、读取预算和格式常量；无Qt依赖，不存放会话状态 */
class SplitViewerCoreConfig
{
public:
    static const uint8_t PngSignature[8];
    static const uint8_t PngIend[4];
    static const uint8_t ConfigChunk[4];
    static const char LegacyMarker[35];
    // 默认舞台宽高比
    static const double kDefaultStageAspect;
    // 最小舞台宽高比
    static const double kMinimumStageAspect;
    // 新分屏默认比例
    static const double kDefaultSplitRatio;
    // 最小分屏比例
    static const double kMinimumSplitRatio;
    // 最大分屏比例
    static const double kMaximumSplitRatio;
    // 新层起点
    static const double kLayerOrigin;
    // 新层错位步长
    static const double kLayerOffset;
    // 图层错位循环数
    static const int32_t kLayerOffsetPeriod;
    // 新层归一化宽高
    static const double kLayerExtent;
    // 新层起点上限
    static const double kLayerOriginLimit;
    // 独立图层默认起点
    static const double kDefaultLayerStart;
    // 独立图层默认终点
    static const double kDefaultLayerEnd;
    // 配置恢复最小图层宽度
    static const double kProfileMinimumLayerWidth;
    // 配置恢复最小图层高度
    static const double kProfileMinimumLayerHeight;
    // 普通滚轮缩放倍率
    static const double kZoomStep;
    // 精细滚轮缩放倍率
    static const double kFineZoomStep;
    // 缩放绝对下限
    static const double kMinimumScale;
    // 相对适配的最小缩放
    static const double kMinimumFitScale;
    // 相对适配的最大缩放
    static const double kMaximumFitScale;
    // 极小适配比例时最小倍率范围
    static const double kMinimumZoomRange;
    // 读取树深度上限
    static const int32_t kMaximumTreeDepth;
    // 单树读取节点上限
    static const int32_t kMaximumNodes;
    // 配置读取图层上限
    static const int32_t kMaximumLayers;
    // UTF16配置内容字节上限
    static const int32_t kMaximumProfileBytes;
};