#pragma once
#include "DialogShow.h"

class PopDialog : public DialogShow
{
    Q_OBJECT
public:
    PopDialog();
    /** 设置默认控件，含有关闭按钮，标题，分割线，框大小，背景颜色，背景边框
    @param [in] dialogWidth 窗口宽度
    @param [in] dialogHeight 窗口高度
    */
    void init();

    /** 关闭窗口
    @param [in] result 窗口返回值
    */
    void done(int32_t result);

    /** 设置窗口标题
    */
    void setWindowTiTle(const QString& title,
                        const QColor& color = QColor(163, 175, 191, 255),
                        int32_t fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        int32_t origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);

private slots:
    void onNcActiveChanged(const bool& ncActive);

private:
    bool m_highLight;
    Separator* m_separator;
};