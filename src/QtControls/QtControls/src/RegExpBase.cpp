#include "RegExpBase.h"
#include <QComboBox>
#include <QLineEdit>
#include <stdint.h>
#include <QEvent>
#include "core/CStringManager.h"
#include "ComboBox.h"

//"^500|(?!00|[5-9][0-9]{2})(?:[0-9]{1,3})$"
void RegExpBase::setRegularExpression(QComboBox* comboBox, const QString& regularStr, const QString& preText)
{
	if (comboBox == nullptr || regularStr.isEmpty())
	{
		return;
	}

	RE::RegExpAttri attri;
	m_mapAttri[comboBox] = attri;
	comboBox->setEditable(true);
	m_mapAttri[comboBox].m_regExp.setPattern(regularStr);
	m_mapAttri[comboBox].m_validator.setRegExp(m_mapAttri[comboBox].m_regExp);
	comboBox->setValidator(&(m_mapAttri[comboBox].m_validator));
	comboBox->installEventFilter(comboBox->parent());
	comboBox->lineEdit()->installEventFilter(comboBox->parent());
	m_mapAttri[comboBox].m_preText = preText;
}

void RegExpBase::RegExpEventFilter(QObject* target, QEvent* eve)
{
	if (target == nullptr || eve == nullptr)
	{
		return;
	}

	switch (eve->type())
	{
	case QEvent::MouseButtonPress:
	{
		comboBoxPressMap(target);
		break;
	}
	case QEvent::FocusOut:
	{
		comboBoxFocusOutMap(target);
		break;
	}
	case QEvent::MouseButtonRelease:
	{
		if (!m_mapAttri.empty())
		{
			QWidget* widget = ((QWidget*)m_mapAttri.begin()->first->parent());
			if (target == widget)
			{
				widget->setFocus();
			}
		}
		break;
	}
	default:
		break;
	}
}

void RegExpBase::onSetEditTextFinish(QComboBox* comboBox)
{
	auto itAttri = m_mapAttri.find(comboBox);
	if (itAttri == m_mapAttri.end())
	{
		return;
	}
	itAttri->second.m_finished = true;
	std::wstring editedStr = comboBox->currentText().toStdWString();
	if (editedStr != L"")
	{
		CStringManager::Replace(editedStr, itAttri->second.m_preText.toStdWString(), L"");
		itAttri->second.m_currentText = QString::fromStdWString(editedStr);
	}
}

void RegExpBase::onSetTextChanged(QComboBox* comboBox, const QString& text)
{
	auto itAttri = m_mapAttri.find(comboBox);
	if (itAttri == m_mapAttri.end())
	{
		return;
	}

	std::wstring wstrText = text.toStdWString();
	CStringManager::Replace(wstrText, itAttri->second.m_preText.toStdWString(), L"");
	QString qstrText = QString::fromStdWString(wstrText);

	itAttri->second.m_regExp.indexIn(qstrText);
	QStringList res = itAttri->second.m_regExp.capturedTexts();
	QString capturedText;
	int32_t index = -1;
	while (index++ != res.size() - 1)
	{
		capturedText += res[index];
	}
	bool ok = (capturedText == qstrText);

	index = -1;
	while (index++ != comboBox->count() - 1)
	{
		if (text == comboBox->itemText(index))
		{
			ok = true;
		}
	}

	if (!ok && qstrText != "")
	{
		itAttri->second.m_currentText = QString::fromStdWString(comboBox->itemText(0).toStdWString());
		comboBox->setCurrentIndex(0);
	}
}

void RegExpBase::comboBoxPressMap(QObject* target)
{
	for (auto itAttri = m_mapAttri.begin(); itAttri != m_mapAttri.end(); ++itAttri)
	{
		comboBoxPress(target, itAttri->first, itAttri->second.m_preText, itAttri->second.m_currentText, itAttri->second.m_finished);
	}
}

void RegExpBase::comboBoxPress(QObject* target,
							   QComboBox* comboBox,
							   const QString& addString,
							   QString& curText,
							   bool& finish)
{
	if (target == (QObject*)comboBox->lineEdit() && finish)
	{
		finish = false;
		if (curText != "" && comboBox->currentText() != comboBox->itemText(0))
		{
			comboBox->setCurrentText(curText);
		}
		else
		{
			if (comboBox->currentText().toStdWString() != L"")
			{
				std::wstring wcurText = comboBox->currentText().toStdWString();
				CStringManager::Replace(wcurText, addString.toStdWString(), L"");
				curText = QString::fromStdWString(wcurText);
				comboBox->setCurrentText(QString::fromStdWString(L""));
			}
		}
	}
}

void RegExpBase::comboBoxFocusOutMap(QObject* target)
{
	for (auto itAttri = m_mapAttri.begin(); itAttri != m_mapAttri.end(); ++itAttri)
	{
		comboBoxFocusOut(target, itAttri->first, itAttri->second.m_preText, itAttri->second.m_currentText, itAttri->second.m_finished);
	}
}

void RegExpBase::comboBoxFocusOut(QObject* target,
								  QComboBox* comboBox,
								  const QString& addString,
								  QString& curText,
								  bool& finish)
{
	if (target == comboBox)
	{
		finish = true;
		if (comboBox->currentText() == QString::fromStdWString(L""))
		{
			if (curText != comboBox->itemText(0))
			{
				bool add = true;
				int32_t index = -1;
				while (index++ != comboBox->count() - 1)
				{
					if (curText == comboBox->itemText(index))
					{
						add = false;
					}
				}
				if (add && !curText.isEmpty())
				{
					comboBox->setCurrentText(addString + curText);
				}
				else
				{
					comboBox->setCurrentText(curText);
				}
			}
			else
			{
				comboBox->setCurrentText(curText);
			}
		}
		else if (comboBox->currentText() == comboBox->itemText(0))
		{
			comboBox->setCurrentText(comboBox->itemText(0));
		}
		else
		{
			std::wstring wcurText = comboBox->currentText().toStdWString();

			bool add = true;
			int32_t index = -1;
			while (index++ != comboBox->count() - 1)
			{
				if (wcurText == comboBox->itemText(index).toStdWString())
				{
					add = false;
				}
			}
			if (add && wcurText != L"")
			{
				CStringManager::Replace(wcurText, addString.toStdWString(), L"");
				comboBox->setCurrentText(addString + QString::fromStdWString(wcurText));
			}
			else
			{
				comboBox->setCurrentText(QString::fromStdWString(wcurText));
			}
		}
		curText.clear();
		comboBox->clearFocus();
	}
}