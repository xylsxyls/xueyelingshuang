#include "TipDialog.h"
#include "Label.h"

int32_t TipDialog::popTipDialog(const QString& title,
							 const QString& tip,
							 const QString& buttonText,
							 int32_t done,
							 QDialog* parent,
							 int32_t timeOut)
{
	TipDialog dlg(title, tip, buttonText, done);
	dlg.setResponseHighlightDialog(parent);
	return dlg.exec(timeOut);
}

TipDialog::TipDialog(const QString& title,
					 const QString& tip,
					 const QString& buttonText,
					 int32_t done)
{
	m_title->setText(title);
	m_tip = addLabel(tip, QRect(43, 51, width() - 43 * 2, 60), QColor(205, 213, 225, 255));
	m_tip->setWordWrap(true);
	m_tip->setAlignment(m_tip->alignment() | Qt::AlignHCenter);
	m_accept = addButton(buttonText, QRect((width() - 116) / 2, 127, 116, 22), done);
}