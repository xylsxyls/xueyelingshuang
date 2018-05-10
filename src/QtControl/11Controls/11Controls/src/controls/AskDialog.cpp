#include "AskDialog.h"
#include "Label.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

AskDialog::AskDialog() :
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
    initAcceptButton(m_accept);
    resize(340, 165);
}

void AskDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_tip, tip, QColor(205, 213, 225, 255), 12);
}

void AskDialog::setAcceptButton(const QString& acceptText, DialogResult acceptDone)
{
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252), acceptDone, 14);
}

void AskDialog::setIgnoreButton(const QString& ignoreText, DialogResult ignoreDone)
{
    setPopButtonConfig(m_ignore, ignoreText, QColor(201, 211, 252), ignoreDone, 14);
}

void AskDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_accept->setGeometry(QRect((width() - 116 * 2) / 3, 127, 116, 22));
    m_ignore->setGeometry(QRect((width() - 116 * 2) / 3 * 2 + 116, 127, 116, 22));
    m_tip->setGeometry(QRect(43, 40, width() - 43 * 2, 83));
}

bool AskDialog::check()
{
    return m_tip != nullptr &&
        m_accept != nullptr &&
        m_ignore != nullptr &&
        PopDialog::check();
}
