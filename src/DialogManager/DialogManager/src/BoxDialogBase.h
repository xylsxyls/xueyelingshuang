#pragma once
#include "PopDialog.h"

/** 账号相关弹框基类，复用账号弹框的背景绘制和基础布局检查
*/
class BoxDialogBase : public PopDialog
{
public:
	/** 构造函数
	*/
	BoxDialogBase();

protected:
	/** 绘制账号弹框背景
	@param [in] eve 绘制事件
	*/
	void paintEvent(QPaintEvent* eve);

    /** 保留给子类扩展的尺寸变化处理
    @param [in] eve 窗口大小变化事件
    */
    void resizeEvent(QResizeEvent* eve);

    /** 检查基类控件是否有效
    @return 返回true表示基类控件有效
    */
    bool check();
};