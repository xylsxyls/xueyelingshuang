#pragma once
#include "SplitViewerCore/SplitViewerCoreAPI.h"
#include <QtCore/QString>
#include <QtCore/QByteArray>
/** Qt文件适配；格式规则由Core负责，失败时不替换原文件或文档
*/
class SplitViewerProfileStore
{
public:
    /** 原子保存配置和PNG预览，失败返回false
    @param [in] path 文件路径
    @param [in] document 文档模型
    @param [in] thumbnail PNG缩略图
    @return 原子保存配置和PNG预览，失败返回false是否成功
    */
    static bool write(const QString& path, const SplitViewerCoreDocument& document, const QByteArray& thumbnail);

    /** 读取并验证配置；失败时保留document并返回可呈现的错误
    @param [in] path 文件路径
    @param [in,out] document 文档模型
    @param [in,out] error 供界面显示的错误信息
    @param [in] maximumBytes 本次复制的输入大小预算
    @param [in] readError 本次读取失败文案
    @param [in] invalidError 本次格式失败文案
    @param [in] limitError 本次预算失败文案
    @return 是否成功提交完整文档
    */
    static bool read(const QString& path, SplitViewerCoreDocument& document, QString& error, qint64 maximumBytes,
        const QString& readError, const QString& invalidError, const QString& limitError);
};