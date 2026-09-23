#pragma once
#include "DialogManager/DialogManagerAPI.h"

/** 注册 PdfReader 专用内容区，工厂所有权交给 DialogManager。 */
class PdfReaderDialogFactory : public CustomViewFactory
{
public:
    /** 创建内容区。
    @param [in] param 弹窗参数。
    @return 参数类型匹配时返回新内容区，否则返回空。
    */
    CustomViewBase* createView(const DialogParam& param) override;
};
