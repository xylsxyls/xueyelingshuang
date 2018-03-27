#include "RPGThreeWidget.h"
#include "Label.h"
#include "ContentLabel.h"

RPGThreeWidget::RPGThreeWidget(QWidget* parent):
QWidget(parent),
m_titleLeft(nullptr),
m_titleRight(nullptr),
m_separator(nullptr),
m_upTitleOne(nullptr),
m_upTitleTwo(nullptr),
m_upTitleThree(nullptr),
m_downTitleOne(nullptr),
m_downTitleTwo(nullptr),
m_downTitleThree(nullptr),
m_upContentOne(nullptr),
m_upContentTwo(nullptr),
m_upContentThree(nullptr),
m_downContentOne(nullptr),
m_downContentTwo(nullptr),
m_downContentThree(nullptr),
m_upError(nullptr),
m_downError(nullptr)
{
	m_titleLeft = new Label(this);
	m_titleRight = new Label(this);
	m_separator = new Label(this);
	m_upTitleOne = new Label(this);
	m_upTitleTwo = new Label(this);
	m_upTitleThree = new Label(this);
	m_downTitleOne = new Label(this);
	m_downTitleTwo = new Label(this);
	m_downTitleThree = new Label(this);
	m_upContentOne = new ContentLabel(this);
	m_upContentTwo = new ContentLabel(this);
	m_upContentThree = new ContentLabel(this);
	m_downContentOne = new ContentLabel(this);
	m_downContentTwo = new ContentLabel(this);
	m_downContentThree = new ContentLabel(this);
	m_upError = new Label(this);
	m_downError = new Label(this);

	if (m_upError != nullptr && m_downError != nullptr)
	{
		m_upError->setVisible(false);
		m_downError->setVisible(false);
		QObject::connect(this, &RPGThreeWidget::error, m_upError, &Label::setVisible);
		QObject::connect(this, &RPGThreeWidget::error, m_downError, &Label::setVisible);
	}
}

void RPGThreeWidget::init(const QString& titleLeft,
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
	m_titleRight->setText(titleRight);
	m_titleLeft->setTextColor(QColor("#ffd080"));
	m_titleLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_titleRight->setTextColor(QColor("#ffffff"));
	m_titleRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_titleLeft->setTextOrigin(18);
	m_titleRight->setTextRightOrigin(11);

	if (header.size() != 6 || value.size() != 6 || progress.size() != 6)
	{
		return;
	}
	m_upTitleOne->setText(header[0]);
	m_upTitleTwo->setText(header[1]);
	m_upTitleThree->setText(header[2]);
	m_downTitleOne->setText(header[3]);
	m_downTitleTwo->setText(header[4]);
	m_downTitleThree->setText(header[5]);
	m_upContentOne->setUpText(value[0]);
	m_upContentTwo->setUpText(value[1]);
	m_upContentThree->setUpText(value[2]);
	m_downContentOne->setUpText(value[3]);
	m_downContentTwo->setUpText(value[4]);
	m_downContentThree->setUpText(value[5]);

	m_upContentOne->setDownText(progress[0]);
	m_upContentTwo->setDownText(progress[1]);
	m_upContentThree->setDownText(progress[2]);
	m_downContentOne->setDownText(progress[3]);
	m_downContentTwo->setDownText(progress[4]);
	m_downContentThree->setDownText(progress[5]);

	if (progress[0] != "")
	{
		m_upContentOne->setTwoLine();
		if (CStringManager::Find(progress[0].toStdString(), "+") != -1)
		{
			m_upContentOne->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[0].toStdString(), "-") != -1)
		{
			m_upContentOne->setDownColor(QColor("#8de32c"));
		}
	}
	if (progress[1] != "")
	{
		m_upContentTwo->setTwoLine();
		if (CStringManager::Find(progress[1].toStdString(), "+") != -1)
		{
			m_upContentTwo->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[1].toStdString(), "-") != -1)
		{
			m_upContentTwo->setDownColor(QColor("#8de32c"));
		}
	}
	if (progress[2] != "")
	{
		m_upContentThree->setTwoLine();
		if (CStringManager::Find(progress[2].toStdString(), "+") != -1)
		{
			m_upContentThree->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[2].toStdString(), "-") != -1)
		{
			m_upContentThree->setDownColor(QColor("#8de32c"));
		}
	}
	if (progress[3] != "")
	{
		m_downContentOne->setTwoLine();
		if (CStringManager::Find(progress[3].toStdString(), "+") != -1)
		{
			m_downContentOne->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[3].toStdString(), "-") != -1)
		{
			m_downContentOne->setDownColor(QColor("#8de32c"));
		}
	}
	if (progress[4] != "")
	{
		m_downContentTwo->setTwoLine();
		if (CStringManager::Find(progress[4].toStdString(), "+") != -1)
		{
			m_downContentTwo->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[4].toStdString(), "-") != -1)
		{
			m_downContentTwo->setDownColor(QColor("#8de32c"));
		}
	}
	if (progress[5] != "")
	{
		m_downContentThree->setTwoLine();
		if (CStringManager::Find(progress[5].toStdString(), "+") != -1)
		{
			m_downContentThree->setDownColor(QColor("#ee584d"));
		}
		else if (CStringManager::Find(progress[5].toStdString(), "-") != -1)
		{
			m_downContentThree->setDownColor(QColor("#8de32c"));
		}
	}

	m_upError->setText(QStringLiteral("±¾¾Ö»ý·ÖÊý¾ÝÒì³£"));
	m_downError->setText(QStringLiteral("±¾¾Ö»ý·ÖÊý¾ÝÒì³£"));

	m_upError->setTextColor(QColor("#ff2800"));
	m_downError->setTextColor(QColor("#ff2800"));

	m_titleLeft->setBackgroundColor(QColor("#232630"));
	m_titleRight->setBackgroundColor(QColor("#232630"));

	m_separator->setBackgroundColor(QColor("#f9e879"));

	m_upContentOne->setBackgroundColor(QColor("#313541"));
	m_upContentTwo->setBackgroundColor(QColor("#313541"));
	m_upContentThree->setBackgroundColor(QColor("#313541"));
	m_downContentOne->setBackgroundColor(QColor("#313541"));
	m_downContentTwo->setBackgroundColor(QColor("#313541"));
	m_downContentThree->setBackgroundColor(QColor("#313541"));

	m_upTitleOne->setBackgroundColor(QColor("#232630"));
	m_upTitleTwo->setBackgroundColor(QColor("#232630"));
	m_upTitleThree->setBackgroundColor(QColor("#232630"));
	m_downTitleOne->setBackgroundColor(QColor("#232630"));
	m_downTitleTwo->setBackgroundColor(QColor("#232630"));
	m_downTitleThree->setBackgroundColor(QColor("#232630"));

	m_upError->setBackgroundColor(QColor("#313541"));
	m_downError->setBackgroundColor(QColor("#313541"));

	m_titleLeft->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_titleRight->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_upContentOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upContentTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upContentThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_upTitleOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upTitleTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upTitleThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_upError->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downError->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_titleLeft->setFontSize(12);
	m_titleRight->setFontSize(12);

	m_upContentOne->setFontSize(12);
	m_upContentTwo->setFontSize(12);
	m_upContentThree->setFontSize(12);
	m_downContentOne->setFontSize(12);
	m_downContentTwo->setFontSize(12);
	m_downContentThree->setFontSize(12);

	m_upTitleOne->setFontSize(12);
	m_upTitleTwo->setFontSize(12);
	m_upTitleThree->setFontSize(12);
	m_downTitleOne->setFontSize(12);
	m_downTitleTwo->setFontSize(12);
	m_downTitleThree->setFontSize(12);

	m_upError->setFontSize(12);
	m_downError->setFontSize(12);

	m_upTitleOne->setAlignment(Qt::AlignCenter);
	m_upTitleTwo->setAlignment(Qt::AlignCenter);
	m_upTitleThree->setAlignment(Qt::AlignCenter);
	m_downTitleOne->setAlignment(Qt::AlignCenter);
	m_downTitleTwo->setAlignment(Qt::AlignCenter);
	m_downTitleThree->setAlignment(Qt::AlignCenter);

	m_upError->setAlignment(Qt::AlignCenter);
	m_downError->setAlignment(Qt::AlignCenter);

	m_upTitleOne->setTextColor(QColor("#969db6"));
	m_upTitleTwo->setTextColor(QColor("#969db6"));
	m_upTitleThree->setTextColor(QColor("#969db6"));
	m_downTitleOne->setTextColor(QColor("#969db6"));
	m_downTitleTwo->setTextColor(QColor("#969db6"));
	m_downTitleThree->setTextColor(QColor("#969db6"));

	m_upContentOne->setUpColor(QColor("#ffffff"));
	m_upContentTwo->setUpColor(QColor("#ffffff"));
	m_upContentThree->setUpColor(QColor("#ffffff"));
	m_downContentOne->setUpColor(QColor("#ffffff"));
	m_downContentTwo->setUpColor(QColor("#ffffff"));
	m_downContentThree->setUpColor(QColor("#ffffff"));
}

void RPGThreeWidget::setError(bool isError)
{
	emit error(isError);
}

void RPGThreeWidget::resizeEvent(QResizeEvent* eve)
{
	QWidget::resizeEvent(eve);

	if (!check())
	{
		return;
	}

	int32_t RPGWidth = width();
	int32_t RPGHeight = height();

	m_separator->setGeometry(QRect(0, TITLE_HEIGHT, RPGWidth, SEPARATOR_HEIGHT).adjusted(-1, 0, 1, 0));

	m_upTitleOne->setGeometry(QRect(0 * RPGWidth / COLUMN, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_upTitleTwo->setGeometry(QRect(1 * RPGWidth / COLUMN, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_upTitleThree->setGeometry(QRect(2 * RPGWidth / COLUMN, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));

	int32_t downTop = (RPGHeight - TITLE_HEIGHT + SEPARATOR_HEIGHT) / 2 + TITLE_HEIGHT + SEPARATOR_HEIGHT;
	m_downTitleOne->setGeometry(QRect(0 * RPGWidth / COLUMN, downTop, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_downTitleTwo->setGeometry(QRect(1 * RPGWidth / COLUMN, downTop, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_downTitleThree->setGeometry(QRect(2 * RPGWidth / COLUMN, downTop, RPGWidth / COLUMN, UP_DOWN_TITLE_HEIGHT).adjusted(-1, 0, 1, 0));

	int32_t contentHeight = downTop - TITLE_HEIGHT - SEPARATOR_HEIGHT - UP_DOWN_TITLE_HEIGHT;
	int32_t upContentTop = downTop - contentHeight;
	m_upContentOne->setGeometry(QRect(0 * RPGWidth / COLUMN, upContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));
	m_upContentTwo->setGeometry(QRect(1 * RPGWidth / COLUMN, upContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));
	m_upContentThree->setGeometry(QRect(2 * RPGWidth / COLUMN, upContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));

	int32_t downContentTop = downTop + UP_DOWN_TITLE_HEIGHT;
	m_downContentOne->setGeometry(QRect(0 * RPGWidth / COLUMN, downContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));
	m_downContentTwo->setGeometry(QRect(1 * RPGWidth / COLUMN, downContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));
	m_downContentThree->setGeometry(QRect(2 * RPGWidth / COLUMN, downContentTop, RPGWidth / COLUMN, contentHeight).adjusted(-1, -1, 1, 1));

	m_titleLeft->setGeometry(QRect(0, 0, RPGWidth / 2, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));
	m_titleRight->setGeometry(QRect(RPGWidth / 2, 0, RPGWidth / 2, TITLE_HEIGHT).adjusted(-1, 0, 1, 0));

	m_upError->setGeometry(QRect(0, upContentTop, RPGWidth, contentHeight).adjusted(-1, -1, 1, 1));
	m_downError->setGeometry(QRect(0, downContentTop, RPGWidth, contentHeight).adjusted(-1, -1, 1, 1));
}

bool RPGThreeWidget::check()
{
	return m_titleLeft != nullptr &&
		   m_titleRight != nullptr &&
		   m_separator != nullptr &&
		   m_upTitleOne != nullptr &&
		   m_upTitleTwo != nullptr &&
		   m_upTitleThree != nullptr &&
		   m_downTitleOne != nullptr &&
		   m_downTitleTwo != nullptr &&
		   m_downTitleThree != nullptr &&
		   m_upContentOne != nullptr &&
		   m_upContentTwo != nullptr &&
		   m_upContentThree != nullptr &&
		   m_downContentOne != nullptr &&
		   m_downContentTwo != nullptr &&
		   m_downContentThree != nullptr &&
		   m_upError != nullptr &&
		   m_downError != nullptr;
}

