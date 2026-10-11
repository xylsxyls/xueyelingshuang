#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <algorithm>

/** 在Core矩形和Qt绘制坐标间转换，不持有文档或界面资源
*/
class SplitViewerLayoutHelper
{
public:
    /** 查询同树叶子的实际矩形，不受其他图层覆盖影响
    @param [in] root 当前图层根节点
    @param [in] rect 根内容矩形
    @param [in] target 目标叶子
    @param [out] result 叶子矩形
    @param [in] borderVisible 是否绘制分隔线
    @return 是否找到目标
    */
    static bool findLeafRect(
        SplitViewerCoreNode* root, const QRectF& rect, SplitViewerCoreNode* target, QRectF& result, bool borderVisible);

    /** 将Core矩形转换为Qt坐标矩形
    @param [in] value 输入值
    @return 将Core矩形转换为Qt坐标矩形
    */
    static QRectF rectFromCore(const SplitViewerCoreRect& value);

    /** 根据背景板比例和窗口尺寸计算居中区域
    @param [in] size 目标区域尺寸，逻辑像素
    @param [in] aspect 背景板宽高比
    @param [in] fullscreen 是否内部全屏
    @return 根据背景板比例和窗口尺寸计算居中区域
    */
    static QRectF stageRect(const QSize& size, double aspect, bool fullscreen);

    /** 将归一化图层矩形转换为背景板像素坐标
    @param [in] rect 区域矩形
    @param [in] stage 背景板像素矩形
    @return 将归一化图层矩形转换为背景板像素坐标
    */
    static QRectF normalizedToPixel(const SplitViewerCoreRect& rect, const QRectF& stage);

    /** 计算空白分屏中央加号的可点击区域
    @param [in] rect 区域矩形
    @return 计算空白分屏中央加号的可点击区域
    */
    static QRectF plusButtonRect(const QRectF& rect);

    /** 按边框可见性计算内容区域
    @param [in] owner 父内容矩形，与子区域使用同一坐标系
    @param [in] borderVisible 是否显示边框
    @return 按边框可见性计算内容区域
    */
    static QRectF contentRect(const QRectF& owner, bool borderVisible);

    /** 将Core分屏几何转换为Qt矩形
    @param [in] owner 父内容矩形，与子区域使用同一坐标系
    @param [in] node 文档持有的节点
    @param [in,out] first 第一子区域
    @param [in,out] splitter 分割线区域
    @param [in,out] second 第二子区域
    @param [in] borderVisible 是否显示边框
    */
    static void nodeRects(const QRectF& owner, SplitViewerCoreNode* node, QRectF& first, QRectF& splitter,
        QRectF& second, bool borderVisible);
};