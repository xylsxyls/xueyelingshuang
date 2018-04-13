#include "AccountDialog.h"
#include "Label.h"
#include "LineEdit.h"
#include "COriginalButton.h"
#include <QEvent>

AccountDialog::AccountDialog(QWindow* parent, int32_t acceptDone, int32_t ignoreDone) :
BoxDialogBase(340, 296, parent)
{
	m_sep = new Label(this);
	m_sep->setGeometry(QRect(7, 76, width() - 7 * 2, 1));
	m_sep->setBackgroundColor(QColor(74, 89, 128, 255));

	m_registerAlt = addLabel(QString::fromStdWString(L"×¢²áÐ¡ºÅ"), QRect(1, 32, width() - 2, 44), QColor(255, 255, 255, 255));
	m_registerAlt->setAlignment(Qt::AlignCenter);
	m_registerAlt->setFontSize(18);
	m_registerAlt->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));

	m_account = new LineEdit(this);
	m_account->setGeometry(QRect((width() - 248) / 2, 100, 248, 32));
	m_account->setPlaceholderText(QString::fromStdWString(L"ÊäÈëÕËºÅÃû"));
	m_account->setAlignment(Qt::AlignCenter);
	m_account->setFontSize(18);
	m_account->setTextColor(QColor("#5d6a8d"));//QColor("#acc0f5")
	m_account->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
	m_account->setBackgroundColor(QColor(31, 36, 51, 255));
	m_account->setBorderRadius(4);
	m_account->setBorderColor(QColor(67, 81, 117, 255));
	m_account->installEventFilter(this);

	m_errorAccount = addLabel(QString::fromStdWString(L"ÄúÊäÈëµÄÕËºÅÓÐÎó"), QRect(20, 153, width() - 20 * 2, 30), QColor("#e2361f"));
	m_errorAccount->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
	m_errorAccount->setFontSize(18);
	m_errorAccount->setVisible(false);
	QObject::connect(this, &AccountDialog::errorVisible, m_errorAccount, &Label::setEnabled);

	m_accept = addButton(QString::fromStdWString(L"È·¶¨"), QRect((width() - 248) / 2, 229, 100, 28), acceptDone);
	m_accept->setBkgImage("");
	m_accept->setBkgColor();
	m_accept->setBorderWidth(1);
	m_accept->setBorderStyle("solid");
	m_accept->setBorderRadius(4);
	m_accept->setBorderColor(QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255));
	m_accept->setFontSize(13);
	m_accept->setFontColor(QColor(166, 183, 249, 255));
	m_accept->installEventFilter(this);
	QObject::connect(this, &DialogShow::keyboardAccept, this, &AccountDialog::accountAccept);

	m_ignore = addButton(QString::fromStdWString(L"È¡Ïû"), QRect(width() - (width() - 248) / 2 - 100, 229, 100, 28), ignoreDone);
	m_ignore->setBkgImage("");
	m_ignore->setBkgColor();
	m_ignore->setBorderWidth(1);
	m_ignore->setBorderStyle("solid");
	m_ignore->setBorderRadius(4);
	m_ignore->setBorderColor(QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255), QColor(166, 183, 249, 255));
	m_ignore->setFontSize(13);
	m_ignore->setFontColor(QColor(166, 183, 249, 255));

	m_ignore->installEventFilter(this);
}

void AccountDialog::setErrorVisible(bool visible)
{
	emit errorVisible(visible);
}

void AccountDialog::clearAccountEdit()
{
	m_account->clear();
}

QString AccountDialog::accountEditText()
{
	return m_account->text();
}

bool AccountDialog::eventFilter(QObject* tar, QEvent* eve)
{
	bool result = BoxDialogBase::eventFilter(tar, eve);
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
			m_account->setPlaceholderText(QString::fromStdWString(L"ÊäÈëÕËºÅÃû"));
			m_account->setAlignment(Qt::AlignCenter);
			m_account->setTextOrigin(0);
			m_account->setTextColor(QColor("#5d6a8d"));
			m_account->repaint();
		}
	}
	return result;
}

void AccountDialog::accountAccept(QObject* tar, Qt::Key key)
{
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
