#include "TipShowDialog.h"
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"

void TipShowDialog::showTipDialog(int32_t& dialogId,
								  int32_t userType,
								  const QString& title,
								  const QString& tip,
								  const QString& buttonText,
								  int32_t done,
								  int32_t timeOut,
								  bool isCountDownVisible)
{
	TipShowDialog* dlg = new TipShowDialog(userType, title, tip, buttonText, done);
	dlg->show(dialogId, timeOut, isCountDownVisible);
	return;
}

TipShowDialog::TipShowDialog(int32_t userType,
							 const QString& title,
							 const QString& tip,
							 const QString& buttonText,
							 int32_t done)
{
	std::string str = typeid(*this).name();
	CStringManager::Replace(str, "class ", "");
	initForShow(234, 130, str);
	m_userType = userType;
	m_title->setText(title);
	m_tip = addTip(tip, QRect(20, 40, width() - 20 * 2, 50), QColor(205, 213, 225, 255));
	m_accept = addButton(buttonText, QRect((width() - 81) / 2, height() - 13 - 19, 81, 19), done);
	m_accept->setBkgImage("");
	m_accept->setBkgColor(QColor(97, 125, 197, 255), QColor(138, 169, 249, 255), QColor(67, 81, 117, 255));
	m_accept->setFontColor(QColor(255, 255, 255, 255), QColor(255, 255, 255, 255), QColor(188, 199, 226, 255));
	m_accept->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
	m_accept->setBkgMargins(0, 0);
	m_accept->setBorderRadius(4);
}