#include "InputDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

InputDialog::InputDialog() :
m_edit(nullptr),
m_editTip(nullptr),
m_accept(nullptr),
m_editText(nullptr)
{
    m_editTip = new Label(this);
    m_accept = new COriginalButton(this);
    m_edit = new LineEdit(this);

    initAcceptButton(m_accept);
    resize(340, 165);
}

void InputDialog::setLineEdit(const QString& defaultText, QString* editText, int32_t maxLength)
{
    DialogHelper::setLineEdit(m_edit, defaultText, maxLength);
    m_editText = editText;
}

void InputDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_editTip, tip, QColor(205, 213, 225, 255), 12);
}

void InputDialog::setAcceptButton(const QString& acceptText, int32_t acceptDone)
{
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252, 255), acceptDone, 14);
}

void InputDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    m_editTip->setGeometry(QRect(43, 26, width() - 43 * 2, 60));
    m_accept->setGeometry(QRect((width() - 116) / 2, 127, 116, 22));
    m_edit->setGeometry(QRect(52, 74, 234, 26));
}

void InputDialog::closeEvent(QCloseEvent* eve)
{
    if (m_editText != nullptr)
    {
        *m_editText = m_edit->text();
    }
    PopDialog::closeEvent(eve);
}
