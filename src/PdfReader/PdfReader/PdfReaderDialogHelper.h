#pragma once
#include "PdfReaderDialogParam.h"

/** 同步 UI 弹窗入口；只组织参数，所有显示和生命周期由 DialogManager 处理。 */
class PdfReaderDialogHelper
{
public:
    /** 显示提示。
    @param [in] parent 父窗口。
    @param [in] title 标题。
    @param [in] text 提示内容。
    @param [in] config UI 配置。
    */
    static void message(QWidget* parent, const QString& title, const QString& text, const Config& config = Config());
    /** 显示确认框，默认焦点为取消。
    @param [in] parent 父窗口。
    @param [in] title 标题。
    @param [in] text 提示内容。
    @param [in] config UI 配置。
    @return 仅明确点击确认返回 true。
    */
    static bool question(QWidget* parent, const QString& title, const QString& text, const Config& config = Config());
    /** 输入文本或密码。
    @param [in] parent 父窗口。
    @param [in] title 标题。
    @param [in] text 输入提示。
    @param [in,out] value 初始值及确认后的值；取消时保持原值。
    @param [in] password 是否隐藏输入。
    @param [in] config UI 配置。
    @return 确认时返回 true。
    */
    static bool input(QWidget* parent, const QString& title, const QString& text, QString& value, bool password = false, const Config& config = Config());
    /** 选择文件或目录；保存到已有文件时由管理器再次确认。
    @param [in] parent 父窗口。
    @param [in] mode OpenFile、SaveFile 或 Directory。
    @param [in] title 标题。
    @param [in] initial 默认路径。
    @param [in] filter 文件过滤器。
    @param [in] config UI 配置。
    @return 确认后的路径，取消或创建失败返回空。
    */
    static QString file(QWidget* parent, PdfReaderDialogParam::Mode mode, const QString& title,
        const QString& initial, const QString& filter, const Config& config = Config());
private:
    /** 设置父窗口并交给管理器执行。
    @param [in] parent 父窗口。
    @param [in,out] param 创建参数及返回结果。
    @return 仅明确确认返回 true。
    */
    static bool run(QWidget* parent, PdfReaderDialogParam& param);
};
