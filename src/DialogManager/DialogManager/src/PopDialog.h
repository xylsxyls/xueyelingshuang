#pragma once
#include "QtControls/DialogShow.h"

/** 弹框基类
*/
class PopDialog : public DialogShow
{
    Q_OBJECT
public:
    /** 构造函数
    */
    PopDialog();

    /** 设置窗口标题
    @param [in] title 窗口标题
    @param [in] color 窗口标题颜色
    @param [in] fontSize 字体大小
    @param [in] align 对齐方式
    @param [in] origin 当左对齐时窗口标题的偏移量
    @param [in] fontName 字体名
    */
    void setWindowTitle(const QString& title,
                        const QColor& color = QColor(163, 175, 191, 255),
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignCenter,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

    

    /** 设置常态和高亮边框颜色，子类在paintEvent函数内不可以画到边框上
    @param [in] normalColor 常态颜色
    @param [in] highLightColor 高亮颜色
    */
    void setBorderNormalHighLightColor(const QColor& normalColor, const QColor& highLightColor = QColor(0, 0, 0, 0));

public slots:
	/** 按钮点击之后窗口关闭
	*/
    void endDialog();

protected:
    void paintEvent(QPaintEvent* eve);
    void showEvent(QShowEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    void closeEvent(QCloseEvent* eve);
    bool check();

private slots:
    void onNcActiveChanged(const bool& ncActive);
    void onTimeUp();

private:
    void init();

protected:
    Separator* m_separator;

private:
    bool m_highLight;
    QColor m_normalColor;
    QColor m_highLightColor;
};