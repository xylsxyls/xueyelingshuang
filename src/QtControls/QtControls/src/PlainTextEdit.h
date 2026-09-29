#pragma once
#include "ControlShow.h"
#include "ControlSelf.h"
#include "ControlFont.h"
#include "ControlBorder.h"
#include "ControlBackground.h"
#include "QtControlsMacro.h"
#include <QtWidgets/QPlainTextEdit>

/** 纯文本编辑及日志展示控件，复用通用QSS能力，不解析HTML
*/
class QtControlsAPI PlainTextEdit :
    public ControlShow<QPlainTextEdit>,
    public ControlSelf<QPlainTextEdit>,
    public ControlFont<QPlainTextEdit>,
    public ControlBorderForNormalHoverDisabled<QPlainTextEdit>,
    public ControlBackgroundForNormalHoverDisabled<QPlainTextEdit>
{
public:
    /** 构造纯文本控件，默认一像素边框
    @param [in] parent Qt父对象，持有控件生命期
    */
    explicit PlainTextEdit(QWidget* parent = nullptr);
};