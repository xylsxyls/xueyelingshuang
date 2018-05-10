#pragma once
#include <QString>
#include <QRect>
#include <QColor>
#include <stdint.h>
#include <map>
#include "DialogType.h"

#define setPopButtonConfig(button, buttonText, textColor, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, textColor, CGeneralStyle::instance()->platformResourcePath() + "/Dialog/PopupButton.png", acceptDone, fontSize, &m_mapResult, 4, true)
#define setNotifyButtonConfig(button, buttonText, acceptDone, fontSize) DialogHelper::setButton(this, button, buttonText, QColor(255, 255, 255, 255), "", acceptDone, fontSize, &m_mapResult, 0, false)

class Label;
class COriginalButton;
class QSplitter;
class LineEdit;
class CPasswordInputBox;
class DialogShow;
class QWidget;
class QWindow;
class Separator;
/** 帮助窗口创建控件
*/
class DialogHelper
{
public:
    /** 新增标签文字并设置
    @param [in] label 控件指针
    @param [in] text 文本内容
    @param [in] textColor 字体颜色
    @param [in] fontSize 字体大小
    @return 返回标签指针
    */
    static void setLabel(Label* label, const QString& text, const QColor& textColor, int32_t fontSize);

    /** 新增提示文字并设置（多行）
    @param [in] tip 控件指针
    @param [in] text 文本内容
    @param [in] textColor 字体颜色
    @param [in] fontSize 字体大小
    */
    static void setTip(Label* tip, const QString& text, const QColor& textColor, int32_t fontSize);

    /** 新增按钮并设置，按钮点击后会关闭窗口
    @param [in] dialog 窗口指针
    @param [in] button 按钮指针
    @param [in] text 按钮文本内容
    @param [in] textColor 按钮文字颜色
    @param [in] backgroundImage 按钮背景图片路径
    @param [in] result 按下后的返回值
    @param [in] fontSize 字体大小
    @param [in] mapResult 按钮返回值的存储容器
    @param [in] imageMargin 图片起始位置
    @param [in] isPop 是否是弹窗的按钮
    */
    static void setButton(DialogShow* dialog,
                          COriginalButton* button,
                          const QString& text,
                          const QColor& textColor,
                          const QString& backgroundImage,
                          DialogResult result,
                          int32_t fontSize,
                          std::map<QWidget*, DialogResult>* mapResult,
                          int32_t imageMargin,
                          bool isPop);

    /** 添加分割线
    @param [in] separator 分割线控件指针
    @param [in] isHorizon 是否水平
    @param [in] upColor 上层颜色
    @param [in] downColor 下层颜色
    */
    static void setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor);

    /** 添加单行文本框
    @param [in] lineEdit 单行文本框指针
    @param [in] defaultText 默认输入内容
    @param [in] maxLength 文本框可输入的最大长度
    */
    static void setLineEdit(LineEdit* lineEdit, const QString& defaultText = "", int32_t maxLength = -1);

    /** 添加密码框
    @param [in] passwordInputBox 密码框指针
    @param [in] defaultText 默认输入内容
    @param [in] maxLength 文本框可输入的最大长度
    */
    static void setPasswordInputBox(CPasswordInputBox* passwordInputBox, const QString& defaultText = "", int32_t maxLength = -1);

    /** 激活临时父窗口
    @param [in] 需要激活窗口的子窗口句柄
    */
    void activeTransientParentWindow(QWindow* window);
};