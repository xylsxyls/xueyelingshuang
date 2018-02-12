#include "AskShowDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include "NotifyDialogManager.h"

void AskShowDialog::showAskDialog(int32_t& dialogId,
								  int32_t userType,
								  const QString& title,
								  const QString& tip,
								  const QString& acceptText,
								  int32_t acceptDone,
								  const QString& ignoreText,
								  int32_t ignoreDone,
								  int32_t timeOut,
								  bool isCountDownVisible)
{
	AskShowDialog* dlg = new AskShowDialog(userType, title, tip, acceptText, acceptDone, ignoreText, ignoreDone);
	dlg->show(dialogId, timeOut, isCountDownVisible);
	QObject::connect(dlg, &AskShowDialog::dialogDone, &(NotifyDialogManager::instance()), &NotifyDialogManager::onDialogDone);
	return;
}

AskShowDialog::AskShowDialog(int32_t userType,
							 const QString& title,
							 const QString& tip,
							 const QString& acceptText,
							 int32_t acceptDone,
							 const QString& ignoreText,
							 int32_t ignoreDone)
{
	std::string str = typeid(*this).name();
	CStringManager::Replace(str, "class ", "");
	initForShow(212, 118, str);
	m_userType = userType;
	m_title->setText(title);
	setWindowTitle(title);
	m_tip = addTip(tip, QRect(20, 35, width() - 20 * 2, 50), QColor(205, 213, 225, 255));
	m_tip->setFontSize(13);
	m_accept = addButton(acceptText, QRect(31, height() - 13 - 19, (width() - 31 * 2 - 13) / 2, 19), acceptDone);
	m_accept->setBkgImage("");
	m_accept->setBkgColor(QColor(97, 125, 197, 255), QColor(138, 169, 249, 255), QColor(67, 81, 117, 255), QColor(97, 125, 197, 255));
	m_accept->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));
	m_accept->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
	m_accept->setBkgMargins(0, 0);
	m_accept->setBorderRadius(4);

	m_ignore = addButton(ignoreText, QRect(31 + 13 + (width() - 31 * 2 - 13) / 2, height() - 13 - 19, (width() - 31 * 2 - 13) / 2, 19), ignoreDone);
	m_ignore->setBkgImage("");
	m_ignore->setBkgColor(QColor(183, 62, 62, 255), QColor(225, 96, 96, 255), QColor(124, 41, 41, 255), QColor(183, 62, 62, 255));
	m_ignore->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));
	m_ignore->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
	m_ignore->setBkgMargins(0, 0);
	m_ignore->setBorderRadius(4);
}