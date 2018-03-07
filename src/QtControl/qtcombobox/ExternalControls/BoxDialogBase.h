#pragma once
#include "DialogShow.h"

class BoxDialogBase : public DialogShow
{
public:
	BoxDialogBase(int32_t dialogWidth, int32_t dialogHeight, QWindow* parent = nullptr);

	void setDialogTitle(const QString& title);

	void paintEvent(QPaintEvent* eve);


};