#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QMap>
#include <QtGui/QImage>

/** Qt图片解码和缓存引用整理，不解释配置协议或操作原生窗口；调用方持有图像结果
*/
class SplitViewerImageHelper
{
public:
    /** 释放当前文档不再引用的图片，保留多个叶子共享的同一路径
    @param [in] document 当前文档，仅GUI线程调用
    @param [in,out] cache 文档图片缓存
    */
    static void pruneUnusedImages(const SplitViewerCoreDocument& document, QMap<QString, QImage>& cache);

    /** 将Core路径转换为Qt路径
    @param [in] value 输入值
    @return 将Core路径转换为Qt路径
    */
    static QString path(const std::wstring& value);

    /** 递归加载图片并清除无法恢复的内容状态
    @param [in] node 文档持有的节点
    @param [in,out] cache 图片缓存
    */
    static void setImageStatus(SplitViewerCoreNode* node, QMap<QString, QImage>& cache);
};