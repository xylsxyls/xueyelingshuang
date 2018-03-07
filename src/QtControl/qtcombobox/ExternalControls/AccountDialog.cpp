#include "AccountDialog.h"
#include "Label.h"

AccountDialog::AccountDialog(QWindow* parent) :
BoxDialogBase(340, 296, parent)
{
	m_sep = new Label(this);
	m_sep->setGeometry(QRect(7, 76, width() - 7 * 2, 1));
	m_sep->setBackgroundColor(QColor(74, 89, 128, 255));

	m_registerAlt = addLabel(QString::fromStdWString(L"×¢²áĞ¡ºÅ"), QRect(1, 32, width() - 2, 44), QColor(255, 255, 255, 255));
	m_registerAlt->setAlignment(Qt::AlignCenter);
	m_registerAlt->setFontSize(18);
	m_registerAlt->setFontFace(QString::fromStdWString(L"Î¢ÈíÑÅºÚ"));
}
