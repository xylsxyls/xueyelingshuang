#include "AccountDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "COriginalButton.h"
#include <QEvent>
#include "DialogHelper.h"
#include "CGeneralStyle.h"

AccountDialog::AccountDialog() :
m_sep(nullptr),
m_registerAlt(nullptr),
m_account(nullptr),
m_errorAccount(nullptr),
m_accept(nullptr),
m_ignore(nullptr)
{
    m_sep = new Label(this);
    m_registerAlt = new Label(this);
    m_account = new LineEdit(this);
    m_errorAccount = new Label(this);
    m_accept = new COriginalButton(this);
    m_ignore = new COriginalButton(this);
    if (!check())
    {
        return;
    }
	
	m_sep->setBackgroundColor(QColor(74, 89, 128, 255));

    DialogHelper::setLabel(m_registerAlt, QString::fromStdWString(L"注册小号"), QColor(255, 255, 255, 255), 18);

	m_account->setPlaceholderText(QString::fromStdWString(L"输入账号名"));
	m_account->setFontSize(18);
	m_account->setTextColor(QColor("#5d6a8d"));//QColor("#acc0f5")
	m_account->setFontFace(QString::fromStdWString(L"微软雅黑"));
	m_account->setBackgroundColor(QColor(31, 36, 51, 255));
	m_account->setBorderRadius(4);
	m_account->setBorderColor(QColor(67, 81, 117, 255));

    DialogHelper::setLabel(m_errorAccount, QString::fromStdWString(L"您输入的账号有误"), QColor("#e2361f"), 18);
	m_errorAccount->setVisible(false);
	QObject::connect(this, &AccountDialog::errorVisible, m_errorAccount, &Label::setEnabled);

    initAcceptButton(m_accept);

    resize(340, 296);
}

void AccountDialog::setErrorVisible(bool visible)
{
	emit errorVisible(visible);
}

void AccountDialog::clearAccountEdit()
{
    if (!check())
    {
        return;
    }
	m_account->clear();
}

QString AccountDialog::accountEditText()
{
    if (!check())
    {
        return "";
    }
	return m_account->text();
}

void AccountDialog::setAcceptDown(int32_t result)
{
    if (!check())
    {
        return;
    }
    setPopButtonConfig(m_accept, QString::fromStdWString(L"确定"), QColor(), result, 13);
    m_accept->setBkgImage("");
    m_accept->setBkgColor();
    m_accept->setBorderWidth(1);
    m_accept->setBorderStyle("solid");
    m_accept->setBorderRadius(4);
    m_accept->setBorderColor(QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255));
    m_accept->setFontColor(QColor(166, 183, 249, 255));
}

void AccountDialog::setIgnoreDown(int32_t result)
{
    if (!check())
    {
        return;
    }
    setPopButtonConfig(m_ignore, QString::fromStdWString(L"取消"), QColor(), result, 13);
    m_ignore->setBkgImage("");
    m_ignore->setBkgColor();
    m_ignore->setBorderWidth(1);
    m_ignore->setBorderStyle("solid");
    m_ignore->setBorderRadius(4);
    m_ignore->setBorderColor(QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255));
    m_ignore->setFontColor(QColor(166, 183, 249, 255));
}

bool AccountDialog::eventFilter(QObject* tar, QEvent* eve)
{
	bool result = BoxDialogBase::eventFilter(tar, eve);
    if (!check())
    {
        return result;
    }
	if (tar == m_account)
	{
		if (eve->type() == QEvent::FocusIn)
		{
			m_account->setPlaceholderText("");
			m_account->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			m_account->setTextOrigin(4);
			m_account->setTextColor(QColor("#acc0f5"));
			m_account->repaint();
		}
		else if (eve->type() == QEvent::FocusOut && m_account->text() == "")
		{
			m_account->setPlaceholderText(QString::fromStdWString(L"输入账号名"));
			m_account->setAlignment(Qt::AlignCenter);
			m_account->setTextOrigin(0);
			m_account->setTextColor(QColor("#5d6a8d"));
			m_account->repaint();
		}
	}
	return result;
}

void AccountDialog::resizeEvent(QResizeEvent* eve)
{
    BoxDialogBase::resizeEvent(eve);
    if (!check())
    {
        return;
    }
    m_sep->setGeometry(QRect(7, 76, width() - 7 * 2, 1));
    m_registerAlt->setGeometry(QRect(1, 32, width() - 2, 44));
    m_account->setGeometry(QRect((width() - 248) / 2, 100, 248, 32));
    m_errorAccount->setGeometry(QRect(20, 153, width() - 20 * 2, 30));
    m_accept->setGeometry(QRect((width() - 248) / 2, 229, 100, 28));
    m_ignore->setGeometry(QRect(width() - (width() - 248) / 2 - 100, 229, 100, 28));
}

void AccountDialog::accountAccept(QObject* tar, Qt::Key key)
{
    if (!check())
    {
        return;
    }
	if (m_account->hasFocus() && key == Qt::Key_Space)
	{
		return;
	}
	if (m_account == tar)
	{
		m_accept->setFocus();
	}
	else if (m_account->hasFocus() == false)
	{
		m_accept->setFocus();
		m_accept->click();
	}
}

bool AccountDialog::check()
{
    return m_sep != nullptr &&
        m_registerAlt != nullptr &&
        m_account != nullptr &&
        m_errorAccount != nullptr &&
        m_accept != nullptr &&
        m_ignore != nullptr &&
        BoxDialogBase::check();
}
