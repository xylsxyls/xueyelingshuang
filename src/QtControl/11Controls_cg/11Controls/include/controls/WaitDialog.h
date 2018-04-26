#pragma once
#include "DialogShow.h"
#include "ControlsMacro.h"
#include "PopDialog.h"

/** �ȴ�������
*/
class WaitDialog : public PopDialog
{
	Q_OBJECT
public:
    WaitDialog();

    void setTip(const QString& tip);

protected:
    void resizeEvent(QResizeEvent* eve);

private:
    bool check();

private:
	Label* m_tip;
};