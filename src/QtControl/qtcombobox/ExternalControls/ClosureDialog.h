#pragma once
#include "BoxDialogBase.h"

class ClosureDialog : public BoxDialogBase
{
public:
	/** 构造函数
	@param [in] parent 父窗口句柄
	*/
	ClosureDialog(QWindow* parent = nullptr);
};