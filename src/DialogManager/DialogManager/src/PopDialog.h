#pragma once
#include "QtControls/DialogShow.h"

/** 模态弹框基类，封装标题栏、关闭按钮、倒计时和边框高亮绘制
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
    /** 绘制弹框背景和边框
    @param [in] eve 绘制事件
    */
    void paintEvent(QPaintEvent* eve);

    /** 弹框显示时激活并置顶窗口
    @param [in] eve 显示事件
    */
    void showEvent(QShowEvent* eve);

    /** 调整标题栏、关闭按钮、分割线和倒计时布局
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 关闭窗口时停止倒计时
    @param [in] eve 关闭事件
    */
    void closeEvent(QCloseEvent* eve);

    /** 检查基类控件是否有效
    @return 返回true表示基类控件有效
    */
    bool check();

private slots:
    /** 处理非客户区激活状态变化，刷新弹框边框高亮
    @param [in] ncActive true表示窗口处于激活状态
    */
    void onNcActiveChanged(const bool& ncActive);

    /** 处理倒计时结束
    */
    void onTimeUp();

private:
    /** 初始化弹框通用控件和信号连接
    */
    void init();

protected:
    // 标题栏和内容区之间的分割线
    Separator* m_separator;

private:
    // 当前边框是否使用高亮颜色
    bool m_highLight;

    // 常态边框颜色
    QColor m_normalColor;

    // 高亮边框颜色
    QColor m_highLightColor;
};