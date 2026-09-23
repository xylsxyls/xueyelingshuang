#pragma once
#include <QString>

/** 管理器弹窗生命周期、输入取消及封装控件的交互回归。 */
class PdfReaderDialogTests
{
public:
    /** 执行 UI 专项回归，失败抛出异常。
    @param [in] input PDF 样本路径。
    @param [in] directory 测试输出目录。
    */
    static void run(const QString& input, const QString& directory);
};
