#pragma once
#include "SplitViewerCoreMacro.h"
#include <stdint.h>
/** 单个Core文档的默认布局、缩放和读取预算；初始化校验后复制，不依赖Qt或应用单例
*/
class SplitViewerCoreAPI SplitViewerCoreConfig
{
public:
    /** 在初始化列表中设置旧版兼容的默认值
    */
    SplitViewerCoreConfig();

    /** 校验有限数、尺寸比例及读取预算的安全范围
    @return 全部配置有效
    */
    bool isValid() const;

public:
    // 默认舞台宽高比
    double m_defaultStageAspect;
    // 最小舞台宽高比
    double m_minimumStageAspect;
    // 新分屏默认比例
    double m_defaultSplitRatio;
    // 最小分屏比例
    double m_minimumSplitRatio;
    // 最大分屏比例
    double m_maximumSplitRatio;
    // 新层起点
    double m_layerOrigin;
    // 新层错位步长
    double m_layerOffset;
    // 图层错位循环数
    int32_t m_layerOffsetPeriod;
    // 新层归一化宽高
    double m_layerExtent;
    // 新层起点上限
    double m_layerOriginLimit;
    // 独立图层默认起点
    double m_defaultLayerStart;
    // 独立图层默认终点
    double m_defaultLayerEnd;
    // 配置恢复最小图层宽度
    double m_profileMinimumLayerWidth;
    // 配置恢复最小图层高度
    double m_profileMinimumLayerHeight;
    // 普通滚轮缩放倍率
    double m_zoomStep;
    // 精细滚轮缩放倍率
    double m_fineZoomStep;
    // 缩放绝对下限
    double m_minimumScale;
    // 相对适配的最小缩放
    double m_minimumFitScale;
    // 相对适配的最大缩放
    double m_maximumFitScale;
    // 极小适配比例时最小倍率范围
    double m_minimumZoomRange;
    // 读取树深度上限
    int32_t m_maximumTreeDepth;
    // 单树读取节点上限
    int32_t m_maximumNodes;
    // 配置读取图层上限
    int32_t m_maximumLayers;
    // UTF16配置内容字节上限
    int32_t m_maximumProfileBytes;
};