#pragma once
#include "SplitViewerCoreAPI.h"
#include <QtCore/QMap>
#include <QtGui/QImage>

class SplitViewerImageHelper
{
public:
    /** 将Core路径转换为Qt路径。
    @param [in] value 输入值
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static QString path(const std::wstring& value);
    /** 递归加载图片并清除无法恢复的内容状态。
    @param [in] node 文档持有的节点
    @param [in,out] cache 图片缓存
    */
    static void setImageStatus(SplitViewerCoreNode* node, QMap<QString, QImage>& cache);
};