#pragma once
#include "SplitViewerCoreDocument.h"
#include <stdint.h>
#include <stddef.h>

/** 计算分割节点的两个子区域和分割线区域
@param [in] owner 父区域
@param [in] node 分割节点
@param [out] first 第一个子区域
@param [out] splitter 分割线区域
@param [out] second 第二个子区域
@param [in] thickness 分割线厚度，与owner使用相同单位
*/
SplitViewerCoreAPI void SplitViewerCoreSplitNodeRects(const SplitViewerCoreRect& owner, const SplitViewerCoreNode* node,
    SplitViewerCoreRect& first, SplitViewerCoreRect& splitter, SplitViewerCoreRect& second, double thickness = 2.0);

/** 限制浮动图层矩形，保持最小尺寸并处于工作区内
@param [in,out] rect 待限制矩形
@param [in] bounds 工作区边界
@param [in] minWidth 最小宽度
@param [in] minHeight 最小高度
*/
SplitViewerCoreAPI void SplitViewerCoreConstrainLayerRect(
    SplitViewerCoreRect& rect, const SplitViewerCoreRect& bounds, double minWidth, double minHeight);

/** 计算图片自动适配比例
@param [in] imageWidth 图片宽度
@param [in] imageHeight 图片高度
@param [in] contentWidth 内容宽度
@param [in] contentHeight 内容高度
@return 返回保持宽高比的适配比例
*/
SplitViewerCoreAPI double SplitViewerCoreFitScale(
    double imageWidth, double imageHeight, double contentWidth, double contentHeight);

/** 将配置模型编码为带UTF-16LE BOM的旧版兼容配置字节
@param [in] document 文档模型
@param [out] bytes 配置字节
@return 返回true表示编码成功
*/
SplitViewerCoreAPI bool SplitViewerCoreSerializeProfile(
    const SplitViewerCoreDocument& document, std::vector<uint8_t>& bytes);

/** 从UTF-16LE配置字节恢复文档模型
@param [in] bytes 配置字节
@param [out] document 文档模型
@return 返回true表示解析成功
*/
SplitViewerCoreAPI bool SplitViewerCoreDeserializeProfile(
    const std::vector<uint8_t>& bytes, SplitViewerCoreDocument& document);

/** 将PNG缩略图和配置字节封装为.sv字节
@param [in] thumbnailPng PNG缩略图
@param [in] configBytes 配置字节
@param [out] packageBytes 封装结果
@return 返回true表示封装成功
*/
SplitViewerCoreAPI bool SplitViewerCoreBuildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes, std::vector<uint8_t>& packageBytes);

/** 从当前或旧版.sv字节中提取配置
@param [in] bytes .sv文件字节
@param [out] configBytes 配置字节
@return 返回true表示提取成功
*/
SplitViewerCoreAPI bool SplitViewerCoreExtractEmbeddedConfig(
    const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes);

/** 按实例字节预算封装.sv，非法配置或超限时保留原输出
@param [in] thumbnailPng PNG缩略图
@param [in] configBytes 配置字节
@param [in,out] packageBytes 封装结果，支持与输入为同一容器
@param [in] config 已校验的实例配置快照
@return true表示已提交完整结果，false时输出不变
*/
SplitViewerCoreAPI bool SplitViewerCoreBuildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes, std::vector<uint8_t>& packageBytes, const SplitViewerCoreConfig& config);

/** 按实例字节预算提取.sv，校验PNG配置块CRC及旧版尾部配置
@param [in] bytes .sv文件字节
@param [in,out] configBytes 提取结果，支持与输入为同一容器
@param [in] config 已校验的实例配置快照
@return true表示已提交完整结果，false时输出不变
*/
SplitViewerCoreAPI bool SplitViewerCoreExtractEmbeddedConfig(
    const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes, const SplitViewerCoreConfig& config);

/** 查找节点的直接父节点，未找到返回空
@param [in] root 树根
@param [in] target 目标节点
@return 父节点，借用至下一次树修改
*/
SplitViewerCoreAPI SplitViewerCoreNode* SplitViewerCoreFindParent(
    SplitViewerCoreNode* root, SplitViewerCoreNode* target);

/** 在当前图层树中选择固定的方向目标
@param [in] root 当前图层根节点
@param [in] source 源叶子，须属于root
@param [in] direction 移动方向
@return 借用的目标叶子，无目标返回空
*/
SplitViewerCoreAPI SplitViewerCoreNode* SplitViewerCoreDirectionalTarget(
    SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreMoveDirection direction);

/** 将内容迁移至同树目标并清空源，不改变分屏结构
@param [in] root 当前图层根节点
@param [in,out] source 有内容的源叶子
@param [in,out] target 目标叶子，调用方已取得覆盖许可
@return 是否迁移成功，失败不改变状态
*/
SplitViewerCoreAPI bool SplitViewerCoreMoveContent(
    SplitViewerCoreNode* root, SplitViewerCoreNode* source, SplitViewerCoreNode* target);

/** 按旧版滚轮步长更新图片缩放
@param [in,out] view 图片视图
@param [in] fitScale 当前适配比例
@param [in] delta Qt滚轮angleDelta单位，120表示一个标准刻度，120为一格
@param [in] fine 是否Ctrl精调
@param [in] config 本实例已校验的缩放配置快照
*/
SplitViewerCoreAPI void SplitViewerCoreZoom(SplitViewerCoreLeafState& view, double fitScale, int32_t delta, bool fine,
    const SplitViewerCoreConfig& config = SplitViewerCoreConfig());

/** 窗口或图层尺寸改变时保持图片相对位置和手动比例
@param [in,out] view 像素单位的视图状态
@param [in] oldWidth 原内容宽
@param [in] oldHeight 原内容高
@param [in] newWidth 新内容宽
@param [in] newHeight 新内容高
*/
SplitViewerCoreAPI void SplitViewerCoreResizeView(
    SplitViewerCoreLeafState& view, double oldWidth, double oldHeight, double newWidth, double newHeight);