#pragma once
#include <QString>

/** 无状态的桌面文件选择结果处理；不承担 Core 保存事务 */
class PdfReaderFileHelper
{
public:
    /** 空路径保持取消；已有 PDF 后缀转小写，否则补 .pdf */
    static QString pdfOutputPath(const QString& path);
};