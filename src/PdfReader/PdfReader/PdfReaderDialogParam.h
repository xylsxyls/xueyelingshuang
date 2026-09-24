#pragma once
#include "DialogManager/DialogManagerAPI.h"
#include "Config.h"
#include <memory>

/** 同步弹窗参数；结果通过共享字符串回传，内容区不借用栈对象。 */
struct PdfReaderDialogParam : public CustomDialogParam
{
    enum { TypeId = CUSTOM_DIALOG_TYPE_BEGIN + 731 };
    enum Mode { Message, Question, Input, OpenFile, SaveFile, Directory };
    Mode mode;
    QString text;
    QString initial;
    QString filter;
    bool password;
    Config config;
    std::shared_ptr<QString> value;

    /** 初始化普通提示框参数和独立结果。 */
    PdfReaderDialogParam() : CustomDialogParam(static_cast<DialogType>(TypeId)),
        mode(Message), password(false), value(new QString)
    {
        m_hasShadow = true;
        m_shadowSize = 2;
        m_titleBarHeight = 32;
    }
};