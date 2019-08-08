#include "InputDialog.h"
#include "QtControls/Label.h"
#include "QtControls/LineEdit.h"
#include "QtControls/COriginalButton.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "QtControls/CGeneralStyle.h"
#include "QtControls/CPasswordInputBox.h"

InputDialog::InputDialog() :
m_edit(nullptr),
m_editTip(nullptr),
m_accept(nullptr),
m_editText(nullptr),
m_passwordInputBox(nullptr),
m_isPassword(false),
m_inputEx(nullptr)
{
    m_editTip = new Label(this);
    m_accept = new COriginalButton(this);
    m_passwordInputBox = new CPasswordInputBox(this);
    m_edit = new LineEdit(this);

    if (!check())
    {
        return;
    }

    m_edit->setFocus();

    addListenKey(Qt::Key_Return);
    addListenKey(Qt::Key_Enter);
	QObject::connect(this, &DialogShow::keyboardAccept, this, &InputDialog::onKeyboardAccept, Qt::QueuedConnection);
    m_acceptButton = m_accept;

    resize(340, 165);
}

void InputDialog::setLineEdit(const QString& defaultText, QString* editText, qint32 maxLength)
{
    if (!check())
    {
        return;
    }
	m_isPassword = false;
    m_passwordInputBox->setVisible(false);
    m_edit->setVisible(true);
    DialogHelper::setLineEdit(m_edit, defaultText, maxLength);
    m_editText = editText;
}

void InputDialog::setPasswordInputBox(const QString& defaultText, QString* editText, qint32 maxLength)
{
    if (!check())
    {
        return;
    }
	m_isPassword = true;
    m_passwordInputBox->setVisible(true);
    m_edit->setVisible(false);
    DialogHelper::setPasswordInputBox(m_passwordInputBox, defaultText, maxLength);
    m_editText = editText;
}

void InputDialog::setTip(const QString& tip)
{
    DialogHelper::setTip(m_editTip, tip, QColor(205, 213, 225, 255), 12);
}

void InputDialog::setAcceptButton(const QString& acceptText, DialogResult acceptDone)
{
    setPopButtonConfig(m_accept, acceptText, QColor(201, 211, 252, 255), acceptDone, 14);
}

void InputDialog::setInputEx(std::vector<InputEx>* inputEx)
{
	if (!check())
	{
		return;
	}
	if (inputEx == nullptr)
	{
		return;
	}
	m_inputEx = inputEx;
	m_edit->hide();
	m_passwordInputBox->hide();

	int32_t index = -1;
	while (index++ != inputEx->size() - 1)
	{
		const InputEx& inputDialogEx = (*inputEx)[index];
		Label* tip = new Label(this);
		DialogHelper::setTip(tip, inputDialogEx.m_tip, QColor(205, 213, 225, 255), 12);
		tip->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		LineEdit* lineEdit = nullptr;
		if (inputDialogEx.m_isPassword)
		{
			lineEdit = new CPasswordInputBox(this);
			DialogHelper::setPasswordInputBox((CPasswordInputBox*)lineEdit, inputDialogEx.m_defaultText, inputDialogEx.m_maxLength);
		}
		else
		{
			lineEdit = new LineEdit(this);
			DialogHelper::setLineEdit(lineEdit, inputDialogEx.m_defaultText, inputDialogEx.m_maxLength);
		}
		m_inputExControls.push_back(std::pair<Label*, LineEdit*>(tip, lineEdit));
	}

	resize(340, 165 + 35 * (inputEx->size() - 1));
}

void InputDialog::resizeEvent(QResizeEvent* eve)
{
    PopDialog::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_editTip->setGeometry(QRect(43, 26, width() - 43 * 2, 60));
    m_accept->setGeometry(QRect((width() - 116) / 2, height() - 38, 116, 22));
    m_edit->setGeometry(QRect(52, 74, 234, 26));
    m_passwordInputBox->setGeometry(QRect(52, 74, 234, 26));
	int32_t index = -1;
	while (index++ != m_inputExControls.size() - 1)
	{
		m_inputExControls[index].first->setGeometry(QRect(16, 74 + index * 35, 100, 26));
		m_inputExControls[index].second->setGeometry(QRect(16 + 74, 74 + index * 35, 234, 26));
	}
}

void InputDialog::closeEvent(QCloseEvent* eve)
{
	if (!check())
	{
		return;
	}
	if (m_inputEx != nullptr)
	{
		int32_t index = -1;
		while (index++ != m_inputExControls.size() - 1)
		{
			(*m_inputEx)[index].m_editText = m_inputExControls[index].second->text();
		}
	}
	else
	{
		if (m_editText != nullptr)
		{
			*m_editText = m_isPassword ? m_passwordInputBox->text() : m_edit->text();
		}
	}
    PopDialog::closeEvent(eve);
}

bool InputDialog::check()
{
    return m_editTip != nullptr &&
           m_accept != nullptr &&
           m_edit != nullptr &&
           m_passwordInputBox != nullptr &&
           PopDialog::check();
}