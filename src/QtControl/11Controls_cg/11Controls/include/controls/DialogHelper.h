#pragma once
#include <QString>
#include <QRect>
#include <QColor>
#include <stdint.h>
#include <map>

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
    @param [in] text 文本内容
    @param [in] rect 标签相对父窗口所在位置
    @param [in] textColor 文字颜色
    @return 返回标签指针
    */
    static void setLabel(Label* label, const QString& text, const QColor& textColor, int32_t fontSize);

    /** 新增提示文字并设置（多行）
    @param [in] text 文本内容
    @param [in] rect 标签相对父窗口所在位置
    @param [in] textColor 文字颜色
    @return 返回标签指针
    */
    static void setTip(Label* tip, const QString& text, const QColor& textColor, int32_t fontSize);

    /** 新增按钮并设置，按钮点击后会关闭窗口
    @param [in] text 按钮内文本内容
    @param [in] rect 按钮相对父窗口所在位置
    @param [in] result 关闭窗口后的返回值
    @return 返回按钮指针
    */
    static void setButton(DialogShow* dialog,
                          COriginalButton* button,
                          const QString& text,
                          const QColor& textColor,
                          const QString& backgroundImage,
                          int32_t result,
                          int32_t fontSize,
                          std::map<QWidget*, int32_t>* mapResult,
                          int32_t imageMargin,
                          bool isPop);

    /** 添加分割线
    @param [in] 分割线控件指针
    @param [in] isHorizon 是否水平
    @param [in] upColor 上层颜色
    @param [in] downColor 下层颜色
    @return 返回分割线指针
    */
    static void setSeparator(Separator* separator, bool isHorizon, const QColor& upColor, const QColor& downColor);

    /** 添加单行文本框
    @param [in] rect 标签相对父窗口所在位置
    @param [in] defaultText 默认显示文字
    @return 返回文本框指针
    */
    static void setLineEdit(LineEdit* lineEdit, const QString& defaultText = "", int32_t maxLength = -1);

    /** 添加密码框
    @param [in] rect 标签相对父窗口所在位置
    @param [in] defaultText 默认显示文字
    @return 返回文本框指针
    */
    //static CPasswordInputBox* addPasswordInputBox(const QRect& rect, const QString& defaultText = "");

    /** 激活临时父窗口
    @param [in] 需要激活窗口的子窗口句柄
    */
    void activeTransientParentWindow(QWindow* window);
};