#include "AskShowDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "CGeneralStyle.h"
#include "NotifyDialogManager.h"
#include "DialogHelper.h"
#include "../core/CSystem.h"

AskShowDialog::AskShowDialog():
m_tip(nullptr),
m_accept(nullptr),
m_ignore(nullptr)
{
    m_tip = new Label(this);
    m_accept = new COriginalButton(this);
    m_ignore = new COriginalButton(this);

    if (!check())
    {
        return;
    }

    init(typeid(*this).name());

    DialogHelper::setTip(m_tip, "", QColor(205, 213, 225, 255), 13);
    
	m_accept->setBkgColor(QColor(97, 125, 197, 255), QColor(138, 169, 249, 255), QColor(67, 81, 117, 255), QColor(97, 125, 197, 255));
	m_accept->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));

	m_ignore->setBkgColor(QColor(183, 62, 62, 255), QColor(225, 96, 96, 255), QColor(124, 41, 41, 255), QColor(183, 62, 62, 255));
	m_ignore->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));

    //initKeyboardAccept(m_accept);
    resize(212, 128);
}

void AskShowDialog::setTip(const QString& tip)
{
    if (!check())
    {
        return;
    }
    m_tip->setText(tip);
}

void AskShowDialog::setAcceptButton(const QString& acceptText, int32_t acceptDone)
{
    setNotifyButtonConfig(m_accept, acceptText, acceptDone, 14);
}

void AskShowDialog::setIgnoreButton(const QString& ignoreText, int32_t ignoreDone)
{
    setNotifyButtonConfig(m_ignore, ignoreText, ignoreDone, 14);
}

void AskShowDialog::resizeEvent(QResizeEvent* eve)
{
    NotifyDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_tip->setGeometry(QRect(20, 35, width() - 20 * 2, 50));
    m_accept->setGeometry(QRect(31, height() - 13 - 19, (width() - 31 * 2 - 13) / 2, 19));
    m_ignore->setGeometry(QRect(31 + 13 + (width() - 31 * 2 - 13) / 2, height() - 13 - 19, (width() - 31 * 2 - 13) / 2, 19));
}

bool AskShowDialog::check()
{
    return m_tip != nullptr &&
        m_accept != nullptr &&
        m_ignore != nullptr &&
        NotifyDialog::check();
}
