#pragma once
#include "PopDialog.h"

/** 账号框基类
*/
class BoxDialogBase : public PopDialog
{
public:
	/** 构造函数
	*/
	BoxDialogBase();

protected:
	void paintEvent(QPaintEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    bool check();
};