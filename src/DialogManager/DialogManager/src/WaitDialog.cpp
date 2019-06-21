#include "WaitDialog.h"
#include "QtControls/Label.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "QtControls/COriginalButton.h"

WaitDialog::WaitDialog():
m_tip(nullptr)
{
    m_tip = new Label(this);
    if (!check())
    {
        return;
    }
    m_exit->setVisible(false);
    setEscAltF4Enable(false);
    resize(340, 165);
}

void WaitDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 12);
}

void WaitDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_tip->setGeometry(QRect(43, 51, width() - 43 * 2, 60));
}

bool WaitDialog::check()
{
    return m_tip != nullptr && PopDialog::check();
}
