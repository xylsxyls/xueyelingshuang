#include "BattleDialog.h"
#include <QPainter>
#include <stdint.h>
#include "Label.h"
#include "../COriginalButton.h"
#include "../CGeneralStyle.h"
#include <QPaintEvent>
#include "RPGFourWidget.h"

BattleDialog::BattleDialog(QWidget* parent):
CW3LModalFrame(parent),
m_exit(nullptr),
m_logo(nullptr),
m_title(nullptr),
m_expand(nullptr)
{
	m_exit = new COriginalButton(this);
	m_logo = new Label(this);
	m_title = new Label(this);
	m_expand = new COriginalButton(this);
	setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);

	m_content = new RPGFourWidget(this);
	m_content->setGeometry(2 + 22, 26, 281, 157);
	QStringList headerFour;
	headerFour.push_back(QStringLiteral("ÍÀÁú"));
	headerFour.push_back(QStringLiteral("ìÅÄ¿"));
	headerFour.push_back(QStringLiteral("ÓñÊ÷"));
	headerFour.push_back(QStringLiteral("É±"));
	headerFour.push_back(QStringLiteral("¹ÖÊÞÉ±"));
	headerFour.push_back(QStringLiteral("¾ÖÉ±"));
	headerFour.push_back(QStringLiteral("ÌÓÅÜ"));
	headerFour.push_back(QStringLiteral("ÆäËû"));
	QStringList valueFour;
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	valueFour.push_back(QStringLiteral("999999"));
	QStringList progressFour;
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral("(+125)"));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral(""));
	progressFour.push_back(QStringLiteral("(+125)"));
	progressFour.push_back(QStringLiteral("(-125)"));
	((RPGFourWidget*)m_content)->init(QStringLiteral("Å·Ë¹Ð¡ÇåÐÂÅ·Ò®"), QStringLiteral("À­¶ûÀîÁ¬½Ü´òÆÆ"), headerFour, valueFour, progressFour);
}

void BattleDialog::init()
{
	if (!check())
	{
		return;
	}

	setCustomerTitleBarHeight(26);
	m_exit->setText("");
	m_exit->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallCloseButton.png", 3, 1, 2, 3, 1);
	m_logo->setBackgroundImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallResultIcon.png", 2, 1, 1, 1, 1);
	m_expand->setText("");
	m_expand->setBkgImage(CGeneralStyle::instance()->war3lobbyResourcePath() + "/Image/GameResultView/Small/SmallExpandButton.png", 3, 1, 2, 3, 1);
	setWindowTitle(QStringLiteral("ÂÌÉ«Ñ­»·È¦"));
	m_logo->raise();
}

void BattleDialog::paintEvent(QPaintEvent* eve)
{
	int32_t titleHeight = customerTitleBarHeight();
	int32_t contentHeight = height() - titleHeight;
	QLinearGradient gradient(QPointF(0, 0), QPoint(0, titleHeight));
	gradient.setColorAt(0.00, QColor("#fff37f"));
	gradient.setColorAt(0.74, QColor("#d1953d"));
	gradient.setColorAt(1.00, QColor("#9b6a51"));
	gradient.setSpread(QGradient::PadSpread);

	QPainter painter(this);

	painter.save();
	
	painter.fillRect(QRect(22, 0, width(), titleHeight), gradient);

	QLinearGradient gradientContent(QPointF(0, 0), QPoint(0, contentHeight));
	gradientContent.setColorAt(0.00, QColor("#d1953d"));
	gradientContent.setColorAt(0.38, QColor("#fff37f"));
	gradientContent.setColorAt(0.74, QColor("#d1953d"));
	gradientContent.setColorAt(1.00, QColor("#9b6a51"));
	gradientContent.setSpread(QGradient::PadSpread);

	painter.fillRect(QRect(22, titleHeight, width(), contentHeight), gradientContent);

	QFont font;
	font.setFamily(QStringLiteral("Î¢ÈíÑÅºÚ"));
	font.setPixelSize(16);
	painter.setFont(font);
	painter.setPen("#313541");
	CGeneralStyle::instance()->drawHtmlTextByLabel(&painter, QRect(20 + 25, 0, width(), customerTitleBarHeight()), windowTitle(), Qt::AlignLeft | Qt::AlignVCenter);
	
	painter.restore();
}

void BattleDialog::resizeEvent(QResizeEvent* eve)
{
	CW3LModalFrame::resizeEvent(eve);

	if (!check())
	{
		return;
	}

	m_exit->setGeometry(256 + 22, 0, 29, 26);
	m_logo->setGeometry(0, 0, 44, 52);
	m_expand->setGeometry(230 + 22, 5, 16, 16);
}

bool BattleDialog::check()
{
	return m_exit != nullptr && m_logo != nullptr && m_expand != nullptr;
}
