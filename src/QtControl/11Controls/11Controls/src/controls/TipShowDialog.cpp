#include "TipShowDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include "NotifyDialogManager.h"
#include "DialogHelper.h"

TipShowDialog::TipShowDialog():
m_tip(nullptr),
m_accept(nullptr)
{
    m_tip = new Label(this);
    m_accept = new COriginalButton(this);

    if (!check())
    {
        return;
    }

    init(typeid(*this).name());

    m_accept->setBkgColor(QColor(97, 125, 197, 255), QColor(138, 169, 249, 255), QColor(67, 81, 117, 255), QColor(97, 125, 197, 255));
    m_accept->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));

    setEscAltF4Enable(false);
    resize(234, 140);
}

void TipShowDialog::setTip(const QString& tip)
{
    if (!check())
    {
        return;
    }
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 13);
}

void TipShowDialog::setAcceptButton(const QString& acceptText, DialogResult acceptDone)
{
    if (!check())
    {
        return;
    }
    setNotifyButtonConfig(m_accept, acceptText, acceptDone, 13);
}

void TipShowDialog::resizeEvent(QResizeEvent* eve)
{
    NotifyDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_tip->setGeometry(QRect(20, 40, width() - 20 * 2, 50));
    m_accept->setGeometry(QRect((width() - 81) / 2, height() - 13 - 19, 81, 19));
}

bool TipShowDialog::check()
{
    return m_tip != nullptr &&
        m_accept != nullptr &&
        NotifyDialog::check();
}
