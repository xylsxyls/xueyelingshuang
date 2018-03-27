#include "RPGFourWidget.h"
#include "Label.h"
#include "ContentLabel.h"

RPGFourWidget::RPGFourWidget(QWidget* parent):
QWidget(parent),
m_titleLeft(nullptr),
m_titleRight(nullptr),
m_separator(nullptr),
m_upTitleOne(nullptr),
m_upTitleTwo(nullptr),
m_upTitleThree(nullptr),
m_upTitleFour(nullptr),
m_downTitleOne(nullptr),
m_downTitleTwo(nullptr),
m_downTitleThree(nullptr),
m_downTitleFour(nullptr),
m_upContentOne(nullptr),
m_upContentTwo(nullptr),
m_upContentThree(nullptr),
m_upContentFour(nullptr),
m_downContentOne(nullptr),
m_downContentTwo(nullptr),
m_downContentThree(nullptr),
m_downContentFour(nullptr),
m_upError(nullptr),
m_downError(nullptr)
{
	m_titleLeft = new Label(this);
	m_titleRight = new Label(this);
	m_separator = new Label(this);
	m_upTitleOne = new Label(this);
	m_upTitleTwo = new Label(this);
	m_upTitleThree = new Label(this);
	m_upTitleFour = new Label(this);
	m_downTitleOne = new Label(this);
	m_downTitleTwo = new Label(this);
	m_downTitleThree = new Label(this);
	m_downTitleFour = new Label(this);
	m_upContentOne = new ContentLabel(this);
	m_upContentTwo = new ContentLabel(this);
	m_upContentThree = new ContentLabel(this);
	m_upContentFour = new ContentLabel(this);
	m_downContentOne = new ContentLabel(this);
	m_downContentTwo = new ContentLabel(this);
	m_downContentThree = new ContentLabel(this);
	m_downContentFour = new ContentLabel(this);
	m_upError = new Label(this);
	m_downError = new Label(this);
}

void RPGFourWidget::init(const QStringList& header, const QStringList& value, const QStringList& progress)
{
	if (!check())
	{
		return;
	}

	if (header.size() != 8 || value.size() != 8 || progress.size() != 8)
	{
		return;
	}
	m_upTitleOne->setText(header[0]);
	m_upTitleTwo->setText(header[1]);
	m_upTitleThree->setText(header[2]);
	m_upTitleFour->setText(header[3]);
	m_downTitleOne->setText(header[4]);
	m_downTitleTwo->setText(header[5]);
	m_downTitleThree->setText(header[6]);
	m_downTitleFour->setText(header[7]);
	m_upContentOne->setUpText(value[0]);
	m_upContentTwo->setUpText(value[1]);
	m_upContentThree->setUpText(value[2]);
	m_upContentFour->setUpText(value[3]);
	m_downContentOne->setUpText(value[4]);
	m_downContentTwo->setUpText(value[5]);
	m_downContentThree->setUpText(value[6]);
	m_downContentFour->setUpText(value[7]);

	m_upContentOne->setDownText(progress[0]);
	m_upContentTwo->setDownText(progress[1]);
	m_upContentThree->setDownText(progress[2]);
	m_upContentFour->setDownText(progress[3]);
	m_downContentOne->setDownText(progress[4]);
	m_downContentTwo->setDownText(progress[5]);
	m_downContentThree->setDownText(progress[6]);
	m_downContentFour->setDownText(progress[7]);

	if (progress[0] != "")
	{
		m_upContentOne->setTwoLine();
	}
	if (progress[1] != "")
	{
		m_upContentTwo->setTwoLine();
	}
	if (progress[2] != "")
	{
		m_upContentThree->setTwoLine();
	}
	if (progress[3] != "")
	{
		m_upContentFour->setTwoLine();
	}
	if (progress[4] != "")
	{
		m_downContentOne->setTwoLine();
	}
	if (progress[5] != "")
	{
		m_downContentTwo->setTwoLine();
	}
	if (progress[6] != "")
	{
		m_downContentThree->setTwoLine();
	}
	if (progress[7] != "")
	{
		m_downContentFour->setTwoLine();
	}

	m_upContentOne->setBackgroundColor(QColor("#313541"));
	m_upContentTwo->setBackgroundColor(QColor("#313541"));
	m_upContentThree->setBackgroundColor(QColor("#313541"));
	m_upContentFour->setBackgroundColor(QColor("#313541"));
	m_downContentOne->setBackgroundColor(QColor("#313541"));
	m_downContentTwo->setBackgroundColor(QColor("#313541"));
	m_downContentThree->setBackgroundColor(QColor("#313541"));
	m_downContentFour->setBackgroundColor(QColor("#313541"));

	m_upTitleOne->setBackgroundColor(QColor("#232630"));
	m_upTitleTwo->setBackgroundColor(QColor("#232630"));
	m_upTitleThree->setBackgroundColor(QColor("#232630"));
	m_upTitleFour->setBackgroundColor(QColor("#232630"));
	m_downTitleOne->setBackgroundColor(QColor("#232630"));
	m_downTitleTwo->setBackgroundColor(QColor("#232630"));
	m_downTitleThree->setBackgroundColor(QColor("#232630"));
	m_downTitleFour->setBackgroundColor(QColor("#232630"));

	m_upContentOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upContentTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upContentThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upContentFour->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downContentFour->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_upTitleOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upTitleTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upTitleThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_upTitleFour->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleOne->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleTwo->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleThree->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));
	m_downTitleFour->setFontFace(QStringLiteral("Î¢ÈíÑÅºÚ"));

	m_upContentOne->setFontSize(12);
	m_upContentTwo->setFontSize(12);
	m_upContentThree->setFontSize(12);
	m_upContentFour->setFontSize(12);
	m_downContentOne->setFontSize(12);
	m_downContentTwo->setFontSize(12);
	m_downContentThree->setFontSize(12);
	m_downContentFour->setFontSize(12);

	m_upTitleOne->setFontSize(12);
	m_upTitleTwo->setFontSize(12);
	m_upTitleThree->setFontSize(12);
	m_upTitleFour->setFontSize(12);
	m_downTitleOne->setFontSize(12);
	m_downTitleTwo->setFontSize(12);
	m_downTitleThree->setFontSize(12);
	m_downTitleFour->setFontSize(12);
}

void RPGFourWidget::resizeEvent(QResizeEvent* eve)
{
	QWidget::resizeEvent(eve);

	if (!check())
	{
		return;
	}

	int32_t RPGWidth = width();
	int32_t RPGHeight = height();

	m_upTitleOne->setGeometry(QRect(0 * RPGWidth / 4, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_upTitleTwo->setGeometry(QRect(1 * RPGWidth / 4, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_upTitleThree->setGeometry(QRect(2 * RPGWidth / 4, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_upTitleFour->setGeometry(QRect(3 * RPGWidth / 4, TITLE_HEIGHT + SEPARATOR_HEIGHT, RPGWidth, UP_DOWN_TITLE_HEIGHT));

	int32_t downTop = (RPGHeight - TITLE_HEIGHT + SEPARATOR_HEIGHT) / 2 + TITLE_HEIGHT + SEPARATOR_HEIGHT;
	m_downTitleOne->setGeometry(QRect(0 * RPGWidth / 4, downTop, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_downTitleTwo->setGeometry(QRect(1 * RPGWidth / 4, downTop, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_downTitleThree->setGeometry(QRect(2 * RPGWidth / 4, downTop, RPGWidth, UP_DOWN_TITLE_HEIGHT));
	m_downTitleFour->setGeometry(QRect(3 * RPGWidth / 4, downTop, RPGWidth, UP_DOWN_TITLE_HEIGHT));

	int32_t contentHeight = downTop - TITLE_HEIGHT - SEPARATOR_HEIGHT - UP_DOWN_TITLE_HEIGHT;
	int32_t upContentTop = downTop - contentHeight;
	m_upContentOne->setGeometry(QRect(0 * RPGWidth / 4, upContentTop, RPGWidth, contentHeight));
	m_upContentTwo->setGeometry(QRect(1 * RPGWidth / 4, upContentTop, RPGWidth, contentHeight));
	m_upContentThree->setGeometry(QRect(2 * RPGWidth / 4, upContentTop, RPGWidth, contentHeight));
	m_upContentFour->setGeometry(QRect(3 * RPGWidth / 4, upContentTop, RPGWidth, contentHeight));

	int32_t downContentTop = downTop + UP_DOWN_TITLE_HEIGHT;
	m_downContentOne->setGeometry(QRect(0 * RPGWidth / 4, downContentTop, RPGWidth, contentHeight));
	m_downContentTwo->setGeometry(QRect(1 * RPGWidth / 4, downContentTop, RPGWidth, contentHeight));
	m_downContentThree->setGeometry(QRect(2 * RPGWidth / 4, downContentTop, RPGWidth, contentHeight));
	m_downContentFour->setGeometry(QRect(3 * RPGWidth / 4, downContentTop, RPGWidth, contentHeight));

	m_titleLeft->setGeometry(QRect(0, 0, RPGWidth / 2, TITLE_HEIGHT));
	m_titleRight->setGeometry(QRect(RPGWidth / 2, 0, RPGWidth / 2, TITLE_HEIGHT));
}

bool RPGFourWidget::check()
{
	return m_titleLeft != nullptr &&
		m_titleRight != nullptr &&
		m_separator != nullptr &&
		m_upTitleOne != nullptr &&
		m_upTitleTwo != nullptr &&
		m_upTitleThree != nullptr &&
		m_upTitleFour != nullptr &&
		m_downTitleOne != nullptr &&
		m_downTitleTwo != nullptr &&
		m_downTitleThree != nullptr &&
		m_downTitleFour != nullptr &&
		m_upContentOne != nullptr &&
		m_upContentTwo != nullptr &&
		m_upContentThree != nullptr &&
		m_upContentFour != nullptr &&
		m_downContentOne != nullptr &&
		m_downContentTwo != nullptr &&
		m_downContentThree != nullptr &&
		m_downContentFour != nullptr &&
		m_upError != nullptr &&
		m_downError != nullptr;
}

