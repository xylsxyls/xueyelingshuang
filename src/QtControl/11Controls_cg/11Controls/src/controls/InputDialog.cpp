#include "InputDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "COriginalButton.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "CGeneralStyle.h"

InputDialog::InputDialog() :
m_edit(nullptr)
{
    initKeyboardAccept(m_accept);
    resize(340, 165);
}

void InputDialog::initLineEdit(const QString& defaultText, QString* editText, bool isPassword, int32_t maxLength)
{
    m_edit = DialogHelper::initLineEdit(this, isPassword, defaultText, maxLength);
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

void InputDialog::done(int result)
{
	*m_editText = m_edit->text();
	PopDialog::done(result);
}

void InputDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    m_editTip->setGeometry(QRect(43, 26, width() - 43 * 2, 60));
    m_accept->setGeometry(QRect((width() - 116) / 2, 127, 116, 22));
    m_edit->setGeometry(QRect(52, 74, 234, 26));
}
