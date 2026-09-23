#pragma once
#include "SplitViewerCoreAPI.h"
#include <QtCore/QString>
#include <QtCore/QByteArray>
/** Qt文件适配；格式规则由Core负责，失败时不替换原文件或文档。 */
class SplitViewerProfileStore
{
public:
    /** 原子保存配置和PNG预览，失败返回false。
    @param [in] path 文件路径
    @param [in] document 文档模型
    @param [in] thumbnail PNG缩略图
    @return 是否满足条件或操作成功。
    */
    static bool write(const QString& path, const SplitViewerCoreDocument& document, const QByteArray& thumbnail);
    /** 读取并验证配置；失败时保留document并返回可呈现的错误。
    @param [in] path 文件路径
    @param [in,out] document 文档模型
    @param [in,out] error 供界面显示的错误信息
    @return 是否满足条件或操作成功。
    */
    static bool read(const QString& path, SplitViewerCoreDocument& document, QString& error);
};