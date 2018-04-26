#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

class BoxDialogBase : public PopDialog
{
public:
	/** ¹¹Ôìº¯Êý
	*/
	BoxDialogBase();

protected:
	void paintEvent(QPaintEvent* eve);
    void resizeEvent(QResizeEvent* eve);
    bool check();
};