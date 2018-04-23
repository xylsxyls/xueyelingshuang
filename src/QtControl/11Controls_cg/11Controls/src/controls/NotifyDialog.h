#pragma once
#include "DialogShow.h"

class NotifyDialog : public DialogShow
{
    Q_OBJECT
public:
    NotifyDialog();
    /** 设置默认控件，含有标题栏
    @param [in] typeName 类型名
    */
    void init(const std::string& typeName);

    /** 关闭窗口
    @param [in] result 窗口返回值
    */
    void done(int32_t result);

    /** 设置窗口标题
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(221, 213, 198, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

protected:
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void end();

protected:
    QPropertyAnimation m_animation;
    QRect m_beginRect;
    QRect m_endRect;
    Label* m_titleBar;
    Label* m_icon;
    bool m_isShow;
};