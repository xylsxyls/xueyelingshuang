#include "RPGContentWidget.h"
#include "Label.h"
#include "ContentLabel.h"

RPGContentWidget::RPGContentWidget(int32_t row, int32_t column, QWidget* parent) :
RPGContentBase(parent),
m_row(row),
m_column(column),
m_titleLeft(nullptr),
m_titleRight(nullptr),
m_separator(nullptr),
m_szTitle(nullptr),
m_szContent(nullptr),
m_szError(nullptr),
m_errorLeft(nullptr),
m_errorAllTitle(nullptr),
m_errorTitle(nullptr),
m_errorAllContent(nullptr)
{
	m_titleLeft = new Label(this);
	m_titleRight = new Label(this);
	m_separator = new Label(this);

	m_szTitle = CreateDyadicArray<Label*>(m_row, m_column);
	m_szContent = CreateDyadicArray<ContentLabel*>(m_row, m_column);
	
	m_szError = new Label*[m_row];

	m_errorTitle = new Label*[m_column];

	m_errorLeft = new Label(this);
	if (m_errorLeft != nullptr)
	{
		m_errorLeft->setVisible(false);
	}

	m_errorAllTitle = new Label(this);
	if (m_errorAllTitle != nullptr)
	{
		m_errorAllTitle->setVisible(false);
	}

	m_errorAllContentTitle = new Label(this);
	if (m_errorAllContentTitle != nullptr)
	{
		m_errorAllContentTitle->setVisible(false);
	}

	m_errorAllContent = new Label(this);
	if (m_errorAllContent != nullptr)
	{
		m_errorAllContent->setVisible(false);
	}

	for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
	{
		for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
		{
			m_szTitle[rowIndex][columnIndex] = new Label(this);
			m_szContent[rowIndex][columnIndex] = new ContentLabel(this);
			if (rowIndex == 0)
			{
				auto& errorTitle = m_errorTitle[columnIndex];
				errorTitle = new Label(this);
				if (errorTitle != nullptr)
				{
					errorTitle->setVisible(false);
				}
			}
		}
		if (m_szError != nullptr)
		{
			auto& szError = m_szError[rowIndex];
			szError = new Label(this);
			if (szError != nullptr)
			{
				szError->setVisible(false);
			}
		}
	}
	raiseError();
	QObject::connect(this, &RPGContentWidget::stateSignal, this, &RPGContentWidget::onState);
}

RPGContentWidget::~RPGContentWidget()
{
	DestroyDyadicArray(m_szTitle, m_row);
	DestroyDyadicArray(m_szContent, m_row);
	if (m_szError != nullptr)
	{
		delete[] m_szError;
	}
}

void RPGContentWidget::init(const QString& titleLeft,
						  const QString& titleRight,
						  const QStringList& header,
						  const QStringList& value,
						  const QStringList& progress)
{
	if (!check())
	{
		return;
	}

	m_titleLeft->setText(titleLeft);
	m_titleLeft->setTextColor(QColor("#ffd080"));
	m_titleLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_titleLeft->setTextOrigin(18);
	m_titleLeft->setBackgroundColor(QColor("#232630"));
	m_titleLeft->setFontSize(12);
	m_titleLeft->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	
	m_titleRight->setText(titleRight);
	m_titleRight->setTextColor(QColor("#ffffff"));
	m_titleRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_titleRight->setTextRightOrigin(11);
	m_titleRight->setBackgroundColor(QColor("#232630"));
	m_titleRight->setFontSize(12);
	m_titleRight->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_separator->setBackgroundColor(QColor("#f9e879"));

	m_errorLeft->setText("--");
	m_errorLeft->setBackgroundColor(QColor("#232630"));
	m_errorLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_errorLeft->setTextOrigin(18);
	m_errorLeft->setTextColor(QColor("#ffffff"));
	m_errorLeft->setFontSize(12);
	m_errorLeft->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_errorAllTitle->setBackgroundColor(QColor("#232630"));

	m_errorAllContentTitle->setBackgroundColor(QColor("#232630"));

	m_errorAllContent->setBackgroundColor(QColor("#232630"));
	m_errorAllContent->setText(QStringLiteral("±¾¾ÖÎªÎÞÐ§¾Ö£¬ÇëÄú¼ÌÐøÓÎÏ·¡£"));
	m_errorAllContent->setTextColor(QColor("#ff2800"));
	m_errorAllContent->setBackgroundColor(QColor("#313541"));
	m_errorAllContent->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_errorAllContent->setFontSize(12);
	m_errorAllContent->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

	for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
	{
		auto& errorTitle = m_errorTitle[columnIndex];
		errorTitle->setText("--");
		errorTitle->setBackgroundColor(QColor("#232630"));
		errorTitle->setAlignment(Qt::AlignCenter);
		errorTitle->setTextColor(QColor("#969db6"));
		errorTitle->setFontSize(12);
		errorTitle->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	}

	for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
	{
		for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
		{
			int32_t index = rowIndex * m_column + columnIndex;
			auto& titlePtr = m_szTitle[rowIndex][columnIndex];
			titlePtr->setBackgroundColor(QColor("#232630"));
			if (index < header.size())
			{
				titlePtr->setText(header[index]);
				titlePtr->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
				titlePtr->setFontSize(12);
				titlePtr->setAlignment(Qt::AlignCenter);
				titlePtr->setTextColor(QColor("#969db6"));
			}

			auto& contentPtr = m_szContent[rowIndex][columnIndex];
			contentPtr->setBackgroundColor(QColor("#313541"));
			if (index < value.size())
			{
				contentPtr->setUpText(value[index]);
				contentPtr->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
				contentPtr->setFontSize(12);
				contentPtr->setUpColor(QColor("#ffffff"));
				if (index < progress.size())
				{
					if (progress[index] != "")
					{
						contentPtr->setDownText(progress[index]);
						contentPtr->setTwoLine();
						if (CStringManager::Find(progress[index].toStdString(), "+") != -1)
						{
							contentPtr->setDownColor(QColor("#ee584d"));
						}
						else if (CStringManager::Find(progress[index].toStdString(), "-") != -1)
						{
							contentPtr->setDownColor(QColor("#8de32c"));
						}
					}
				}
			}
		}
		auto& errorPtr = m_szError[rowIndex];
		errorPtr->setText(QStringLiteral("±¾¾Ö»ý·ÖÊý¾ÝÒì³£"));
		errorPtr->setTextColor(QColor("#ff2800"));
		errorPtr->setBackgroundColor(QColor("#313541"));
		errorPtr->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
		errorPtr->setFontSize(12);
		errorPtr->setAlignment(Qt::AlignCenter);
	}
}

void RPGContentWidget::resizeEvent(QResizeEvent* eve)
{
	if (!check())
	{
		return;
	}

	int32_t RPGWidth = width();
	int32_t RPGHeight = height();
	int32_t titleSepHeight = TITLE_HEIGHT + SEPARATOR_HEIGHT;
	int32_t titleContentHeight = (RPGHeight - titleSepHeight) / m_row;
	int32_t contentHeight = titleContentHeight - CONTENT_TITLE_HEIGHT;

	m_titleLeft->setGeometry(QRect(0, 0, RPGWidth / 2, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_errorLeft->setGeometry(QRect(0, 0, RPGWidth / 2, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_titleRight->setGeometry(QRect(RPGWidth / 2, 0, RPGWidth / 2, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_errorAllTitle->setGeometry(QRect(0, 0, RPGWidth, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_errorAllContentTitle->setGeometry(QRect(0, titleSepHeight, RPGWidth, CONTENT_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));

	m_errorAllContent->setGeometry(QRect(0,
		titleSepHeight + CONTENT_TITLE_HEIGHT,
		RPGWidth,
		RPGHeight - titleSepHeight - CONTENT_TITLE_HEIGHT).adjusted(-1, -1, 1, 1));

	m_separator->setGeometry(QRect(0, TITLE_HEIGHT, RPGWidth, SEPARATOR_HEIGHT).adjusted(-1, 0, 1, 0));

	for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
	{
		for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
		{
			m_szTitle[rowIndex][columnIndex]->setGeometry(QRect(columnIndex * RPGWidth / m_column,
				titleContentHeight * rowIndex + titleSepHeight,
				RPGWidth / m_column,
				CONTENT_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));

			m_szContent[rowIndex][columnIndex]->setGeometry(QRect(columnIndex * RPGWidth / m_column,
				titleContentHeight * rowIndex + titleSepHeight + CONTENT_TITLE_HEIGHT,
				RPGWidth / m_column,
				contentHeight).adjusted(-1, -1, 1, 1));

			if (rowIndex == 0)
			{
				m_errorTitle[columnIndex]->setGeometry(QRect(columnIndex * RPGWidth / m_column,
					titleContentHeight * rowIndex + titleSepHeight,
					RPGWidth / m_column,
					CONTENT_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
			}
		}
		m_szError[rowIndex]->setGeometry(QRect(0,
			titleContentHeight * rowIndex + titleSepHeight + CONTENT_TITLE_HEIGHT,
			RPGWidth,
			contentHeight).adjusted(-1, -1, 1, 1));
	}

	QWidget::resizeEvent(eve);
}

template <typename TypeClass>
TypeClass** RPGContentWidget::CreateDyadicArray(int32_t row, int32_t column)
{
	TypeClass** dyadicArrayPtr = new TypeClass*[row];
	if (dyadicArrayPtr != nullptr)
	{
		for (int32_t index = 0; index < row; ++index)
		{
			dyadicArrayPtr[index] = new TypeClass[column];
			if (dyadicArrayPtr[index] == nullptr)
			{
				for (int32_t errorIndex = 0; errorIndex < index; ++errorIndex)
				{
					delete[] dyadicArrayPtr[errorIndex];
					dyadicArrayPtr[errorIndex] = nullptr;
				}
				dyadicArrayPtr = nullptr;
				break;
			}
		}
	}
	return dyadicArrayPtr;
}

template <typename TypeClass>
void RPGContentWidget::DestroyDyadicArray(TypeClass** classPtr, int32_t row)
{
	if (classPtr == nullptr)
	{
		return;
	}

	for (int32_t index = 0; index < row; index++)
	{
		delete[] classPtr[index];
		classPtr[index] = nullptr;
	}
	delete[] classPtr;
	classPtr = nullptr;
}

void RPGContentWidget::onState(ContentState state)
{
	if (!check())
	{
		return;
	}

	hideError();
	switch (state)
	{
	case NORMAL:
	{
		break;
	}
	case ERROR_VALUE:
	{
		for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
		{
			m_szError[rowIndex]->setVisible(true);
		}
		break;
	}
	case ERROR_HEADER:
	{
		m_errorLeft->setVisible(true);
		for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
		{
			m_errorTitle[columnIndex]->setVisible(true);
		}
		m_errorAllContent->setVisible(true);
		break;
	}
	case ERROR_ALL:
	{
		m_errorAllTitle->setVisible(true);
		m_errorAllContentTitle->setVisible(true);
		m_errorAllContent->setVisible(true);
		for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
		{
			m_szError[rowIndex]->setVisible(true);
		}
		break;
	}
	default:
		break;
	}
}

void RPGContentWidget::hideError()
{
	if (!check())
	{
		return;
	}

	for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
	{
		m_szError[rowIndex]->setVisible(false);
	}
	m_errorLeft->setVisible(false);
	for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
	{
		m_errorTitle[columnIndex]->setVisible(false);
	}
	m_errorAllContent->setVisible(false);
	m_errorAllTitle->setVisible(false);
	m_errorAllContentTitle->setVisible(false);
	m_errorAllContent->setVisible(false);
}

void RPGContentWidget::raiseError()
{
	if (!check())
	{
		return;
	}

	for (int32_t rowIndex = 0; rowIndex < m_row; ++rowIndex)
	{
		m_szError[rowIndex]->raise();
	}
	m_errorLeft->raise();
	for (int32_t columnIndex = 0; columnIndex < m_column; ++columnIndex)
	{
		m_errorTitle[columnIndex]->raise();
	}
	m_errorAllContent->raise();
	m_errorAllTitle->raise();
	m_errorAllContentTitle->raise();
	m_errorAllContent->raise();
}

bool RPGContentWidget::check()
{
	return m_titleLeft != nullptr &&
		   m_titleRight != nullptr &&
		   m_separator != nullptr &&
		   m_szTitle != nullptr &&
		   m_szContent != nullptr &&
		   m_szError != nullptr &&
		   m_errorLeft != nullptr &&
		   m_errorAllTitle != nullptr &&
		   m_errorAllContentTitle != nullptr &&
		   m_errorTitle != nullptr && 
		   m_errorAllContent != nullptr;
}

