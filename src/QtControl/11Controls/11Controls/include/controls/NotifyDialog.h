#pragma once
#include "DialogShow.h"

class NotifyDialog : public DialogShow
{
    Q_OBJECT
public:
    /** 构造函数
    */
    NotifyDialog();

    /** 设置默认控件，含有标题栏
    @param [in] typeName 类型名
    */
    void init(const std::string& typeName);

    /** 开始准备关闭
    */
    void prepareExit();

    /** 设置窗口标题
    @param [in] title 窗口标题
    @param [in] color 窗口标题颜色
    @param [in] fontSize 字体大小
    @param [in] align 对齐方式
    @param [in] origin 当左对齐时窗口标题的偏移量
    @param [in] fontName 字体名
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
    bool check();

private:
    DialogResult buttonResult(COriginalButton* button);

    /** 开始执行关闭动画
    */
    void beginExitAnimation();

private slots:
    void onTimeUp();

protected:
    QPropertyAnimation m_animation;
    QRect m_beginRect;
    QRect m_endRect;
    Label* m_titleBar;
    Label* m_icon;
    bool m_isShow;
};