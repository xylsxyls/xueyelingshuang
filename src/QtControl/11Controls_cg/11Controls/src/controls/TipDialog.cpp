#include "TipDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

TipDialog::TipDialog():
m_tip(nullptr),
m_accept(nullptr)
{
    m_tip = new Label(this);
    m_accept = new COriginalButton(this);
    if (!check())
    {
        return;
    }
    initAcceptButton(m_accept);
    resize(340, 165);
}

void TipDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 12);
}

void TipDialog::setAcceptButton(const QString& acceptText, int32_t acceptDone)
{
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252, 255), acceptDone, 14);
}

void TipDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_tip->setGeometry(QRect(43, 40, width() - 43 * 2, 83));
    m_accept->setGeometry(QRect((width() - 116) / 2, 127, 116, 22));
}

bool TipDialog::check()
{
    return m_tip != nullptr && m_accept != nullptr && PopDialog::check();
}
