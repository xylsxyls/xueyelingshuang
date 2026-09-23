#pragma once
#include "PdfReaderDialogParam.h"

class LineEdit;
class FileDialog;
class PushButton;

/** PdfReader 弹窗内容区；外层窗口的创建、模态、关闭和销毁交给 DialogManager。 */
class PdfReaderDialogView : public CustomViewBase
{
public:
    /** 创建空内容区，内部控件由 Qt 父子关系持有。 */
    PdfReaderDialogView();
    /** 根据强类型参数建立内容区。
    @param [in] param 模式、文本、配置与共享结果。
    @return 类型和参数有效时返回 true。
    */
    bool initView(const DialogParam& param) override;
    /** 返回当前模式建议的窗口大小。 */
    QSize preferredSize() const override;
    /** 返回输入框或默认取消按钮，避免确认框 Enter 意外覆盖。 */
    QWidget* defaultFocusWidget() const override;
private:
    /** 接受输入或提示，并请求管理器关闭窗口。 */
    void acceptValue();
    /** 取消并请求管理器关闭窗口，不写入结果。 */
    void cancel();
private:
    PdfReaderDialogParam m_param;
    LineEdit* m_input;
    FileDialog* m_files;
    PushButton* m_cancel;
    PushButton* m_accept;
};
