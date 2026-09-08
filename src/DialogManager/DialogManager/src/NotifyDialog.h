#pragma once
#include "QtControls/DialogShow.h"

/** 通知框基类，封装右下角滑入滑出动画、标题栏和倒计时关闭逻辑
*/
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
    void setWindowTitle(const QString& title,
                        const QColor& color = QColor(221, 213, 198, 255),
                        qint32 fontSize = 12,
                        Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft,
                        qint32 origin = 0,
                        const QString& fontName = QString::fromStdWString(L"微软雅黑"));

protected:
    /** 通知框显示时启动进入动画
    @param [in] eve 显示事件
    */
    void showEvent(QShowEvent* eve);

    /** 调整通知框基础控件布局并计算动画起止区域
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 动画结束后的关闭处理
    */
    void end();

    /** 检查基类控件是否有效
    @return 返回true表示基类控件有效
    */
    bool check();

private:
    /** 根据按钮指针查找弹框返回值
    @param [in] button 被点击的按钮
    @return 返回按钮对应的弹框结果，未找到时返回ERROR_RESULT
    */
    DialogResult buttonResult(PushButton* button);

    /** 开始执行关闭动画
    */
    void beginExitAnimation();

private slots:
    /** 处理倒计时结束
    */
    void onTimeUp();

protected:
    // 通知框进入和退出动画对象
    QPropertyAnimation m_animation;

    // 通知框显示时的目标位置
    QRect m_beginRect;

    // 通知框隐藏时的目标位置
    QRect m_endRect;

    // 标题栏背景
    Label* m_titleBar;

    // 标题栏图标
    Label* m_icon;

    // 当前是否处于显示状态
    bool m_isShow;

	// 当前是否已经进入退出动画，避免重复触发关闭流程
	bool m_isExiting;
};