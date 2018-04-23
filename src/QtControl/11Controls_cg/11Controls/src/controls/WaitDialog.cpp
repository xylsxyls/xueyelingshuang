#include "WaitDialog.h"
#include "Label.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "COriginalButton.h"

WaitDialog::WaitDialog():
m_tip(nullptr)
{
    m_tip = new Label(this);

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
    m_tip->setGeometry(QRect(43, 51, width() - 43 * 2, 60));
}